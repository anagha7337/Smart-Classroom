#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

const char* topic = "nssce/smartclassroom/group6/test";
const char* commandTopic =
  "nssce/smartclassroom/group6/command";

#define DHT_PIN 15
#define LDR_PIN 34
#define PIR_PIN 27
#define LIGHT_PIN 18
#define FAN_PIN 19

DHTesp dht;
WiFiClient espClient;
PubSubClient client(espClient);

bool autoMode = true;
bool lightOn = false;
bool fanOn = false;

void setLight(bool state) {
  lightOn = state;
  digitalWrite(LIGHT_PIN, lightOn ? HIGH : LOW);
}

void setFan(bool state) {
  fanOn = state;
  digitalWrite(FAN_PIN, fanOn ? HIGH : LOW);
}

// Receive commands from HiveMQ
void callback(char* topicName, byte* payload,
              unsigned int length) {
  if (strcmp(topicName, commandTopic) != 0) {
    return;
  }

  char message[32];
  if (length >= sizeof(message)) {
    Serial.println("Command too long!");
    return;
  }

  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.print("Received command: ");
  Serial.println(message);

  if (strcmp(message, "MANUAL") == 0) {
    autoMode = false;
    Serial.println("Manual control enabled");
  }
  else if (strcmp(message, "AUTO") == 0) {
    autoMode = true;
    Serial.println("Automatic control enabled");
  }
  else if (strcmp(message, "LIGHT_ON") == 0) {
    autoMode = false;
    setLight(true);
  }
  else if (strcmp(message, "LIGHT_OFF") == 0) {
    autoMode = false;
    setLight(false);
  }
  else if (strcmp(message, "FAN_ON") == 0) {
    autoMode = false;
    setFan(true);
  }
  else if (strcmp(message, "FAN_OFF") == 0) {
    autoMode = false;
    setFan(false);
  }
  else {
    Serial.println("Unknown command");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  dht.setup(DHT_PIN, DHTesp::DHT22);

  WiFi.begin(ssid, password, 6);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    if (client.connect("SmartClassroomGroup6")) {
      Serial.println("connected!");

      if (client.subscribe(commandTopic)) {
        Serial.println("Subscribed to command topic");
      }
    } else {
      Serial.print("Failed, state = ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  // Process incoming MQTT commands
  client.loop();

  static unsigned long lastMessage = 0;

  if (millis() - lastMessage >= 5000) {
    lastMessage = millis();

    TempAndHumidity data = dht.getTempAndHumidity();
    int lightLevel = analogRead(LDR_PIN);
    bool motion = digitalRead(PIR_PIN);

    if (isnan(data.temperature) ||
        isnan(data.humidity)) {
      Serial.println("DHT22 reading failed!");
      return;
    }

    // Apply automation only in AUTO mode
    if (autoMode) {
      setLight(motion && lightLevel > 2000);
      setFan(motion && data.temperature >= 28.0);
    }

    char payload[300];

    snprintf(payload, sizeof(payload),
      "{\"temperature\":%.1f,\"humidity\":%.1f,"
      "\"light_level\":%d,\"motion\":%s,"
      "\"light\":%s,\"fan\":%s,\"mode\":\"%s\"}",
      data.temperature,
      data.humidity,
      lightLevel,
      motion ? "true" : "false",
      lightOn ? "true" : "false",
      fanOn ? "true" : "false",
      autoMode ? "AUTO" : "MANUAL"
    );

    Serial.println(payload);

    if (client.publish(topic, payload)) {
      Serial.println("Sensor data published!");
    } else {
      Serial.println("MQTT publish failed!");
    }
  }
}
