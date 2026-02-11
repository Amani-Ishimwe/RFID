#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <SPI.h>

// ───────────── PINS ─────────────
#define SS_PIN  15   // D8
#define RST_PIN 4    // D2

// ───────────── WIFI ─────────────
const char* ssid     = "EdNet";
const char* password = "Huawei@123";

// ───────────── MQTT ─────────────
const char* mqtt_server = "broker.benax.rw";
const uint16_t mqtt_port = 1883;

String team_id = "Krii_pa";
String base_topic = "rfid/" + team_id + "/";

String topic_status  = base_topic + "card/status";
String topic_topup   = base_topic + "card/topup";
String topic_balance = base_topic + "card/balance";

// ───────────── OBJECTS ─────────────
WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 rfid(SS_PIN, RST_PIN);

String pending_uid = "";
uint64_t pending_amount = 0;

MFRC522::MIFARE_Key key = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// ───────────── WIFI CONNECT ─────────────
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());
}

// ───────────── MQTT RECONNECT ─────────────
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    String clientId = "ESP8266-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(topic_topup.c_str());
      Serial.println("Subscribed to: " + topic_topup);
    } else {
      Serial.print("failed rc=");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

// ───────────── MQTT CALLBACK ─────────────
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++)
    message += (char)payload[i];

  Serial.println("Incoming: " + message);

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, message)) {
    Serial.println("JSON Error");
    return;
  }

  pending_uid = doc["uid"].as<String>();
  pending_amount = doc["amount"].as<uint64_t>();

  Serial.println("Topup Pending → " + pending_uid + " + " + String(pending_amount));
}

// ───────────── GET UID ─────────────
String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// ───────────── READ BALANCE ─────────────
uint64_t readBalance() {
  byte buffer[18];
  byte size = sizeof(buffer);

  if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 7, &key, &(rfid.uid)) != MFRC522::STATUS_OK)
    return 0;

  if (rfid.MIFARE_Read(4, buffer, &size) != MFRC522::STATUS_OK) {
    rfid.PCD_StopCrypto1();
    return 0;
  }

  uint64_t balance = 0;
  for (int i = 0; i < 8; i++)
    balance |= ((uint64_t)buffer[i] << (8 * i));

  rfid.PCD_StopCrypto1();
  return balance;
}

// ───────────── WRITE BALANCE ─────────────
bool writeBalance(uint64_t newBalance) {
  byte dataBlock[16] = {0};

  for (int i = 0; i < 8; i++)
    dataBlock[i] = (newBalance >> (8 * i)) & 0xFF;

  if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 7, &key, &(rfid.uid)) != MFRC522::STATUS_OK)
    return false;

  if (rfid.MIFARE_Write(4, dataBlock, 16) != MFRC522::STATUS_OK) {
    rfid.PCD_StopCrypto1();
    return false;
  }

  rfid.PCD_StopCrypto1();
  return true;
}

// ───────────── PUBLISH STATUS ─────────────
void publishStatus(String uid, uint64_t balance) {
  StaticJsonDocument<128> doc;
  doc["uid"] = uid;
  doc["balance"] = balance;

  char json[128];
  serializeJson(doc, json);

  client.publish(topic_status.c_str(), json);
  Serial.println("Published: " + String(json));
}

void publishNewBalance(String uid, uint64_t balance) {
  StaticJsonDocument<128> doc;
  doc["uid"] = uid;
  doc["new_balance"] = balance;

  char json[128];
  serializeJson(doc, json);

  client.publish(topic_balance.c_str(), json);
  Serial.println("New Balance Published");
}

// ───────────── SETUP ─────────────
void setup() {
  Serial.begin(115200);
  delay(200);

  SPI.begin();
  rfid.PCD_Init();

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("System Ready");
}

// ───────────── LOOP ─────────────
void loop() {

  if (!client.connected())
    reconnect();

  client.loop();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(100);
    return;
  }

  String uid = getUID();
  Serial.println("Card UID: " + uid);

  uint64_t balance = readBalance();
  Serial.println("Current Balance: " + String(balance));

  // 🔥 Always publish current status first
  publishStatus(uid, balance);

  // 🔥 Then check if topup matches
  if (pending_uid == uid && pending_amount > 0) {

    Serial.println("Applying topup...");

    uint64_t newBalance = balance + pending_amount;

    if (writeBalance(newBalance)) {
      Serial.println("Write success!");

      // 🔥 READ AGAIN after writing (important!)
      uint64_t verifyBalance = readBalance();

      publishNewBalance(uid, verifyBalance);
      Serial.println("Verified Balance: " + String(verifyBalance));
    } else {
      Serial.println("Write FAILED!");
    }

    pending_uid = "";
    pending_amount = 0;
    delay(2000);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(800);
}
