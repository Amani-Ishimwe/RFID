# RFID Card Top-Up System

This project is an IoT-based RFID payment system that allows users to scan cards, view balances, and top up funds via a web dashboard. It uses an ESP8266 microcontroller for hardware communication and a Node.js server for backend processing, connected via MQTT.

## 🚀 Live Dashboard

Access the live dashboard here:
**[http://157.173.101.159:9234](http:/157.173.101.159:9234)**

## 📂 Project Structure

- **backend/**: Contains the Node.js server (`server.js`) and the Arduino code (`arduino.ino`).
- **public/**: Contains the frontend dashboard (`index.html`).

## 🛠️ Hardware Requirements

- **ESP8266 (NodeMCU)**
- **MFRC522 RFID Module**
- **Jumper Wires & Breadboard**

### Pin Configuration (ESP8266 -> MFRC522)

- **SDA (SS)** -> D8 (GPIO 15)
- **SCK** -> D5 (GPIO 14)
- **MOSI** -> D7 (GPIO 13)
- **MISO** -> D6 (GPIO 12)
- **IRQ** -> (Not Connected)
- **GND** -> GND
- **RST** -> D2 (GPIO 4)
- **3.3V** -> 3.3V

## 💻 Installation & Usage

### 1. Backend Setup

Install dependencies and start the server:

```bash
npm install
node backend/server.js
```

The server will start on port `9234`.

### 2. Hardware Setup

1. Open `backend/arduino.ino` in Arduino IDE.
2. Install the necessary libraries:
   - `PubSubClient` (for MQTT)
   - `MFRC522` (for RFID)
   - `ArduinoJson` (for JSON parsing)
3. Update specific configurations if needed (WiFi credentials, MQTT broker).
4. Upload the code to your ESP8266.

## 📡 MQTT Topics

- `rfid/Krii_pa/card/status`: Publishes card UID and current balance.
- `rfid/Krii_pa/card/topup`: Subscribes to top-up requests.
- `rfid/Krii_pa/card/balance`: Publishes updated balance after top-up.

## 🔗 API Endpoints

- **POST `/topup`**
  - Body: `{ "uid": "CARD_UID", "amount": 500 }`
  - Used to add funds to a specific card.
