const express = require("express");
const mqtt = require("mqtt");
const WebSocket = require("ws");
const http = require("http");
const cors = require("cors");
const path = require("path");

const app = express();
app.use(express.json());
app.use(cors());
app.use(express.static(path.join(__dirname, "public"))); // serve dashboard

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// =============================
// CONFIG
// =============================
const TEAM_ID = "Krii_pa"; 
const MQTT_BROKER = "mqtt://broker.benax.rw:1883"; 

const TOPIC_STATUS  = `rfid/${TEAM_ID}/card/status`;
const TOPIC_TOPUP   = `rfid/${TEAM_ID}/card/topup`;
const TOPIC_BALANCE = `rfid/${TEAM_ID}/card/balance`;

// =============================
// MQTT CLIENT
// =============================
const mqttClient = mqtt.connect(MQTT_BROKER);

mqttClient.on("connect", () => {
    console.log("✅ Connected to MQTT broker");
    mqttClient.subscribe([TOPIC_STATUS, TOPIC_BALANCE], (err) => {
        if (!err) console.log("Subscribed to topics");
    });
});

mqttClient.on("message", (topic, message) => {
    try {
        const data = JSON.parse(message.toString());
        console.log("MQTT Message:", topic, data);

        // Broadcast to all connected browsers
        wss.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(JSON.stringify({ topic, data }));
            }
        });
    } catch (err) {
        console.error("Failed to parse MQTT message:", err);
    }
});

// =============================
// HTTP ROUTES
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
    console.log("Top-up published:", payload);

    res.json({ success: true, message: "Top-up command sent" });
});

// Optional: serve index.html explicitly
app.get("/index.html", (req, res) => {
    res.sendFile(path.join(__dirname, "public", "index.html"));
});

// =============================
// START SERVER
// =============================
const PORT = 9234;
server.listen(PORT, () => {
    console.log(`🚀 Server running at http://localhost:${PORT}`);
});
