const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const mqtt = require("mqtt");
const cors = require("cors");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });
app.use(cors());
app.use(express.json());

const PORT = 9234;

// Store balances per card UID
const cardBalances = {};

// MQTT setup
const mqttClient = mqtt.connect("mqtt://broker.benax.rw");

mqttClient.on("connect", () => {
  console.log("Connected to MQTT broker");
  mqttClient.subscribe("rfid/team_07/card/status");
});

mqttClient.on("message", (topic, message) => {
  const data = JSON.parse(message.toString());
  const { uid, balance } = data;

  // Update backend balance storage
  if (!(uid in cardBalances)) cardBalances[uid] = balance;

  // Send update to all connected WebSocket clients
  const payload = JSON.stringify({ uid, balance: cardBalances[uid] });
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) client.send(payload);
  });
});

wss.on("connection", ws => {
  console.log("WebSocket client connected");
});

// Top-up endpoint
app.post("/topup", (req, res) => {
  const { uid, amount } = req.body;
  if (!uid || !amount) return res.status(400).send("UID and amount required");

  cardBalances[uid] = (cardBalances[uid] || 0) + parseFloat(amount);

  // Notify all WebSocket clients of updated balance
  const payload = JSON.stringify({ uid, balance: cardBalances[uid] });
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) client.send(payload);
  });

  res.send({ uid, balance: cardBalances[uid] });
});

server.listen(PORT, () => {
  console.log(`HTTP & WebSocket server running on port ${PORT}`);
});