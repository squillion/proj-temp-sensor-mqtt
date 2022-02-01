#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include "netconfig.h"
#include "ui.h"


// Network config. Change values in netconfig.h
char wifi_ssid[] = SECRET_WIFI_SSID;
char wifi_pass[] = SECRET_WIFI_PASS;

char mqtt_broker[] = MQTT_SERVER;
int  mqtt_port     = MQTT_PORT;
char mqtt_user[]   = SECRET_MQTT_USER;
char mqtt_pass[]   = SECRET_MQTT_PASS;
char mqtt_topic[]  = MQTT_TOPIC;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// How long to wait before values are too old to display
const long valueExpireTime = 120000;
unsigned long lastReceived = 0;

// Write error to serial and show status on LCD
// Loops forever, it does not return
void writeError(const char* msg) {
  Serial.println(msg);
  uiDisplayError(msg);
  while (1) { }
}

void setupNetwork() {
  // For Adafruit boards or breakout set pins
  WiFi.setPins(SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0, &SPIWIFI);

  // Check for Wifi module
  while (WiFi.status() == WL_NO_MODULE) {
    writeError("Communication with WiFi module failed");
  }

  // Connect to Wifi
  Serial.print("Connecting to Wifi SSID: ");
  Serial.println(wifi_ssid);
  while (WiFi.begin(wifi_ssid, wifi_pass) != WL_CONNECTED) {
    // Keep retrying
    Serial.print(".");
    delay(5000);
  }
  uiDisplayStatus(STATUS_WIFI, STATUS_CONNECTED);
  Serial.println("Wifi connected");

  // Connect to MQTT server
  Serial.print("Connecting to the MQTT server: ");
  Serial.println(mqtt_broker);
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);
  if (!mqttClient.connect(mqtt_broker, mqtt_port)) {
    String err = String("MQTT connection error: ") + mqttClient.connectError();
    writeError(err.c_str());
  }
  uiDisplayStatus(STATUS_MQTT, STATUS_CONNECTED);
  Serial.println("MQTT connected");
}

void setup() {
  //Initialize serial, SPI, display, and network
  Serial.begin(9600);
  delay(100);

  SPI.begin();

  uiInit();

  setupNetwork();

  mqttClient.onMessage(onMqttMessage);
  mqttClient.subscribe(mqtt_topic);
}

void onMqttMessage(int messageSize) {
  // Can get topic with mqttClient.messageTopic()

  // Store the time we received it
  lastReceived = millis();

  // read the message contents
  char data[messageSize + 1] = {0};
  int idx = 0;
  while (mqttClient.available()) {
    data[idx] = mqttClient.read();
    idx++;
  }
  data[idx] = '\0';

  // Parse message as JSON and display values
  DynamicJsonDocument doc(messageSize + 1);
  deserializeJson(doc, data);

  int temp = (int) doc["temperature"];
  uiDisplayTemp(temp);

  int pressure = ((int) doc["pressure"]) / 100;
  uiDisplayPressure(pressure);

  int lux = (int) doc["lux"];
  uiDisplayLux(lux);
}

void loop() {
  // poll the mqtt client to receive data and send keepalives
  mqttClient.poll();

  if (lastReceived) {
    // If we've not received a value in a while then blank the values.
    unsigned long currentMillis = millis();
    if ((currentMillis - lastReceived) >= valueExpireTime) {
      uiClearValues();
      lastReceived = 0;
    }
  }
}
