const express = require('express');
const bodyParser = require('body-parser');
const mqtt = require('mqtt');
const http = require('http');
const socketIo = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);
const port = 3000;

app.use(bodyParser.json());
app.use(express.static('public')); // 提供靜態文件

// const mqttOptions = {
//     username: 'your_username',
//     password: 'your_password'
//   };

// MQTT Broker 地址
const mqttBrokerUrl = 'mqtt://localhost:1883'; // 本地 Mosquitto Broker 地址

// MQTT 客戶端
const mqttClient = mqtt.connect(mqttBrokerUrl);

// 儲存裝置和 IR 信號
let devices = {};
let irCodes = {};
// let actions = {};

mqttClient.on('connect', () => {
  console.log('Connected to MQTT broker');
  mqttClient.subscribe('home/ir/receive/#', (err) => {
    if (!err) {
      console.log('Subscribed to home/ir/receive/#');
    }
  });
});

mqttClient.on('message', (topic, message) => {
  const deviceId = topic.split('/').pop();
  const irCode = message.toString();

  if (!irCodes[deviceId]) {
    irCodes[deviceId] = [];
  }
  if (!irCodes[deviceId].includes(irCode)) {
    irCodes[deviceId].push(irCode);
    console.log('Received IR code: ', irCode);

    // 通知客戶端新的 IR 代碼
  } else {
    console.log('Duplicate IR code:', irCode);
  }
  io.emit('new-ir-code', { deviceId, irCode});
});

// API 端點：新增裝置
app.post('/api/devices', (req, res) => {
  const { deviceId, deviceName } = req.body;
  devices[deviceId] = deviceName;
  res.send({ status: 'success', devices });
});

// API 端點：獲取裝置列表
app.get('/api/devices', (req, res) => {
  res.send({ devices });
});

// API 端點：發送 IR 信號
app.post('/api/devices/:deviceId/send', (req, res) => {
  const { deviceId } = req.params;
  const { irCode } = req.body;

  mqttClient.publish(`home/ir/send/${deviceId}`, irCode);
  res.send({ status: 'success' });
});

// API 端點：獲取裝置的 IR 信號
app.get('/api/devices/:deviceId/ir-codes', (req, res) => {
  const { deviceId } = req.params;
  console.log('deviceId:', deviceId);
  res.send({ irCodes: irCodes[deviceId] || [] });
});

io.on('connection', (socket) => {
  console.log('A client connected:', socket.id);
});

server.listen(3000, () => {
  console.log(`中央伺服器運行在 http://localhost:${port}`);
});
