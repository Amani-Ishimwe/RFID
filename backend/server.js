const express = require("express");
const mqtt = require("mqtt");
const WebSocket = require("ws");
const http = require("http");
const cors = require("cors");

const app = express();
app.use(express.json());
app.use(cors());
app.use(express.static("public"));

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// =============================
// CONFIG
// =============================
const TEAM_ID = "team07";
const MQTT_BROKER = "mqtt://broker.benax.rw:1883"; // local mosquitto

const TOPIC_STATUS = `rfid/${TEAM_ID}/card/status`;
const TOPIC_TOPUP = `rfid/${TEAM_ID}/card/topup`;
const TOPIC_BALANCE = `rfid/${TEAM_ID}/card/balance`;

// =============================
// MQTT
// =============================
const mqttClient = mqtt.connect(MQTT_BROKER);

mqttClient.on("connect", () => {
    console.log("✅ Connected to MQTT");
    mqttClient.subscribe(TOPIC_STATUS);
    mqttClient.subscribe(TOPIC_BALANCE);
});

mqttClient.on("message", (topic, message) => {
    const data = JSON.parse(message.toString());
    console.log("MQTT Message:", topic, data);

    // Broadcast to all connected browsers
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify({
                topic,
                data
            }));
        }
    });
});

// =============================
// HTTP ROUTE
// =============================
app.post("/topup", (req, res) => {
    const { uid, amount } = req.body;

    if (!uid || !amount) {
        return res.status(400).json({ error: "Missing uid or amount" });
    }

    const payload = {
        uid,
        amount: parseInt(amount)
    };

    mqttClient.publish(TOPIC_TOPUP, JSON.stringify(payload));

    res.json({ success: true, message: "Top-up command sent" });
});

// =============================
server.listen(3000, () => {
    console.log("🚀 Server running at http://localhost:3000");
});
