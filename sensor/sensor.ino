#include <ArduinoMqttClient.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_VEML7700.h>
#include "netconfig.h"

// Sensors to retreive data from
Adafruit_BMP280 bmp;
Adafruit_VEML7700 veml = Adafruit_VEML7700();

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

// Interval for sending messages in milliseconds
const long interval = 10000;
unsigned long previousMillis = 0;

// Blink LED to show error
#define ERROR_BMP "-..."
#define ERROR_VEML "...-"
#define ERROR_WIFI ".--"
#define ERROR_MQTT "--"

// Write error to serial and show status on LED
// Loops forever, it does not return
void writeError(const char* code, const char* msg) {
  Serial.println(msg);

  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);

  const char *p = code;
  while (true) {
    switch (*p) {
      case '.':
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        p++;
        break;
      case '-':
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1500);
        digitalWrite(LED_BUILTIN, LOW);
        p++;
        break;
      case ' ':
        delay(500);
        p++;
        break;
      default:
        delay(2000);
        p = code;
        break;
    }

    delay(1000);
  }
}

void setup_sensors()
{
  unsigned status;

  // Setup BMP280 Sensor
  status = bmp.begin();
  if (!status) {
    writeError(ERROR_BMP, "Could not find BMP280 sensor");
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  // Setup VEML7700
  status = veml.begin();
  if (!status) {
    writeError(ERROR_VEML, "Could not find VEML7700 sensor");
  }

  veml.setGain(VEML7700_GAIN_1);
  veml.setIntegrationTime(VEML7700_IT_800MS);
  veml.setLowThreshold(10000);
  veml.setHighThreshold(20000);
  veml.interruptEnable(true);
}

void setupNetwork() {
  // For Adafruit boards or breakout set pins
  // WiFi.setPins(SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0, &SPIWIFI);

  // Check for Wifi module
  while (WiFi.status() == WL_NO_MODULE) {
    writeError(ERROR_WIFI, "Communication with WiFi module failed");
  }

  // Connect to Wifi
  Serial.print("Connecting to Wifi SSID: ");
  Serial.println(wifi_ssid);
  while (WiFi.begin(wifi_ssid, wifi_pass) != WL_CONNECTED) {
    // Keep retrying
    Serial.print(".");
    delay(5000);
  }
  Serial.println("Wifi connected");

  // Connect to MQTT server
  Serial.print("Connecting to the MQTT server: ");
  Serial.println(mqtt_broker);
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);
  if (!mqttClient.connect(mqtt_broker, mqtt_port)) {
    String err = String("MQTT connection error: ") + mqttClient.connectError();
    writeError(ERROR_MQTT, err.c_str());
  }
  Serial.println("MQTT connected");
}

void setup() {
  // Setup LED and turn off
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialize serial
  Serial.begin(9600);
  delay(100);

  // Initialize SPI
  SPI.begin();

  // Setup sensors
  setup_sensors();

  // Connect to network
  setupNetwork();
}

void loop() {
  // poll the mqtt client to receive data and send keepalives
  mqttClient.poll();

  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= interval) {
    // Only sending data every interval
    previousMillis = currentMillis;

    // Read sensor values
    String tempValue = String(bmp.readTemperature());
    String pressureValue = String(bmp.readPressure());
    String luxValue = String(veml.readLux());

    // Format the values into JSON
    char data[1024] = {0};
    sprintf(data, "{ \"temperature\":%s, \"pressure\":%s, \"lux\":%s }",
            tempValue.c_str(),
            pressureValue.c_str(),
            luxValue.c_str());

    // Send the data to the topic
    mqttClient.beginMessage(mqtt_topic);
    mqttClient.print(data);
    mqttClient.endMessage();
  }
}
