
const SENSOR_TOPIC =
  "nssce/smartclassroom/group6/test";

const COMMAND_TOPIC =
  "nssce/smartclassroom/group6/command";

// Secure WebSocket connection to the public broker
const client = mqtt.connect(
  "wss://broker.hivemq.com:8884/mqtt",
  {
    clientId:
      "SC_Dashboard_" +
      Math.random().toString(16).slice(2, 10),
    clean: true,
    connectTimeout: 10000,
    reconnectPeriod: 3000
  }
);

const $ = (id) => document.getElementById(id);

client.on("connect", () => {
  $("connection").textContent = "Connected";
  $("connection").style.background = "#166534";

  client.subscribe(SENSOR_TOPIC, (error) => {
    if (error) {
      $("feedback").textContent =
        "Could not subscribe to sensor data.";
    }
  });
});

client.on("reconnect", () => {
  $("connection").textContent = "Reconnecting...";
});

client.on("offline", () => {
  $("connection").textContent = "Offline";
  $("connection").style.background = "#92400e";
});

client.on("error", (error) => {
  $("feedback").textContent =
    "MQTT connection error: " + error.message;
});

client.on("message", (topic, message) => {
  if (topic !== SENSOR_TOPIC) return;

  try {
    const data = JSON.parse(message.toString());

    $("temperature").textContent =
      Number(data.temperature).toFixed(1);

    $("humidity").textContent =
      Number(data.humidity).toFixed(1);

    $("lightLevel").textContent = data.light_level;

    $("motion").textContent =
      data.motion ? "Motion detected" : "No motion";

    $("lightStatus").textContent =
      data.light ? "ON" : "OFF";

    $("fanStatus").textContent =
      data.fan ? "ON" : "OFF";

    $("mode").textContent = data.mode || "Unknown";

    $("lastUpdate").textContent =
      "Last updated: " + new Date().toLocaleTimeString();

    $("feedback").textContent = "";
  } catch (error) {
    $("feedback").textContent =
      "Received invalid sensor data.";
  }
});

function sendCommand(command) {
  if (!client.connected) {
    $("feedback").textContent =
      "Not connected to MQTT. Try again shortly.";
    return;
  }

  client.publish(COMMAND_TOPIC, command, (error) => {
    if (error) {
      $("feedback").textContent =
        "Command publishing failed.";
    } else {
      $("feedback").textContent =
        "Command sent: " + command +
        ". Waiting for updated status...";
    }
  });
}
