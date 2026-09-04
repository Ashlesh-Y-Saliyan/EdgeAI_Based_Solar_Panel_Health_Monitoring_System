#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// -------- Edge Impulse library --------
#include <Solar_panel_monitoring_system_inferencing.h>

// -------- WiFi Credentials --------
const char* WIFI_SSID     = "AK";
const char* WIFI_PASSWORD = "12345678";

// -------- MQTT Broker (HiveMQ Cloud) --------
const char* mqtt_server   = "fa16b0b48f14436db82ab35c49e2a7e5.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_user     = "hivemq.webclient.1785473626767";
const char* mqtt_password = "Wm9,50BiAd*I&KTg1f@j";

// This MUST match the topic your teammate's Node-RED "mqtt in" node subscribes to
const char* pubTopic = "solar/telemetry";
const char* MQTT_CLIENT_ID = "ESP32_SolarPanel_Node1";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// -------- Pin Definitions --------
#define LDR_PIN      32
#define VOLTAGE_PIN  34
#define ONE_WIRE_BUS 4

// -------- DS18B20 --------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Voltage Sensor (0-25V Module)
const float VOLTAGE_DIVIDER_RATIO = 5.0;

// ------------------- WiFi Setup -------------------
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

// ------------------- MQTT Setup -------------------
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to HiveMQ Cloud...");
    if (client.connect(MQTT_CLIENT_ID, mqtt_user, mqtt_password)) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 3 seconds...");
      delay(3000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  sensors.begin();

  connectWiFi();

  // NOTE: setInsecure() skips certificate validation - fine for a
  // student/demo project, but not recommended for production use.
  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Solar Monitoring System Started...");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  // -------- Temperature --------
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // -------- LDR --------
  int lightValue = analogRead(LDR_PIN);

  // -------- Voltage --------
  int rawVoltage = analogRead(VOLTAGE_PIN);
  float adcVoltage = rawVoltage * (3.3 / 4095.0);
  float panelVoltage = adcVoltage * VOLTAGE_DIVIDER_RATIO;

  if (panelVoltage < 0.20)
    panelVoltage = 0;

  // -------- Estimated Current --------
  float current = 0;
  if (panelVoltage > 0) {
    current = (panelVoltage / 5.0) * 0.36;
    if (current > 0.36)
      current = 0.36;
  }

  // -------- Estimated Power --------
  float power = panelVoltage * current;

  // -------- Print raw values --------
  Serial.println("--------------------------------");
  Serial.print("Temperature : "); Serial.print(temperature); Serial.println(" C");
  Serial.print("Light : "); Serial.println(lightValue);
  Serial.print("Voltage : "); Serial.print(panelVoltage, 2); Serial.println(" V");
  Serial.print("Estimated Current : "); Serial.print(current, 3); Serial.println(" A");
  Serial.print("Estimated Power : "); Serial.print(power, 2); Serial.println(" W");

  // -------- Run Edge Impulse AI classification --------
  float features[] = { panelVoltage, current, temperature, (float)lightValue };

  ei_impulse_result_t result = { 0 };
  signal_t signal;
  numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

  String predictedLabel = "Healthy";
  float confidence = 0;

  if (res == EI_IMPULSE_OK) {
    float maxValue = 0;
    const char* maxLabel = "Healthy";
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      if (result.classification[ix].value > maxValue) {
        maxValue = result.classification[ix].value;
        maxLabel = result.classification[ix].label;
      }
    }
    predictedLabel = String(maxLabel);
    confidence = maxValue * 100;

    Serial.println("--- AI Prediction ---");
    Serial.print("Predicted Condition : "); Serial.println(predictedLabel);
    Serial.print("Confidence : "); Serial.print(confidence, 1); Serial.println(" %");
  } else {
    Serial.print("ERROR running classifier: ");
    Serial.println(res);
  }

  // -------- Publish to MQTT (field names match the Node-RED function node) --------
  String payload = "{";
  payload += "\"voltage\":" + String(panelVoltage, 2) + ",";
  payload += "\"current\":" + String(current, 3) + ",";
  payload += "\"power\":" + String(power, 2) + ",";
  payload += "\"temp\":" + String(temperature, 1) + ",";
  payload += "\"light\":" + String(lightValue) + ",";
  payload += "\"status\":\"" + predictedLabel + "\",";
  payload += "\"confidence\":" + String(confidence, 1);
  payload += "}";

  client.publish(pubTopic, payload.c_str());
  Serial.println("Published: " + payload);

  delay(2000);
}
