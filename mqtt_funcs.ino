#include "mqtt_funcs.h"
#include "defines.h"
#include "weathertypes.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include "SparkFunBME280.h"
#include <DallasTemperature.h>
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"

extern BME280 tempSensor;
extern bool BME_ok;
extern uint8_t dallasCount;
extern DeviceAddress dallasAddresses[];
extern DallasTemperature dallas_temperature;
extern SFEWeatherMeterKit weatherMeterKit;
extern float raintoday;
extern float lasthourrain;
extern float maxwind4wu;
extern int maxwinddir4wu;

static WiFiClient mqttWifiClient;
static PubSubClient mqttClient(mqttWifiClient);
static Config *mqttConfig = nullptr;
static bool discoveryPublished = false;
static unsigned long lastMqttPublish = 0;
static unsigned long lastMqttReconnectAttempt = 0;

static bool dallasTempValidLocal(float tempC) {
  return tempC > -55.0f && tempC < 85.0f;
}

static void mqttBuildTopic(char *out, size_t outLen, const char *suffix) {
  snprintf(out, outLen, "%s/%s", mqttConfig->mqtt.base_topic, suffix);
}

static void publishDiscoverySensor(const char *objectId, const char *name,
                                   const char *valueTemplate, const char *unit,
                                   const char *deviceClass, const char *stateClass) {
  if (!mqttConfig) {
    return;
  }

  char topic[160];
  char stateTopic[96];
  mqttBuildTopic(stateTopic, sizeof(stateTopic), "state");
  snprintf(topic, sizeof(topic), "%s/sensor/%s_%s/config",
           mqttConfig->mqtt.discovery_prefix,
           mqttConfig->mqtt.base_topic,
           objectId);

  JsonDocument doc;
  doc["name"] = name;
  doc["uniq_id"] = String(mqttConfig->mqtt.base_topic) + "_" + objectId;
  doc["stat_t"] = stateTopic;
  doc["val_tpl"] = valueTemplate;
  if (unit && unit[0]) {
    doc["unit_of_meas"] = unit;
  }
  if (deviceClass && deviceClass[0]) {
    doc["dev_cla"] = deviceClass;
  }
  if (stateClass && stateClass[0]) {
    doc["stat_cla"] = stateClass;
  }
  JsonObject device = doc["dev"].to<JsonObject>();
  device["ids"][0] = mqttConfig->mqtt.base_topic;
  device["name"] = "Weather Station";
  device["mf"] = "SparkFun MicroMod";
  device["mdl"] = "Weather Carrier";
  device["sw"] = FIRMWARE_VERSION;

  char payload[512];
  size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n > 0 && n < sizeof(payload)) {
    mqttClient.publish(topic, payload, true);
  }
}

static void publishHomeAssistantDiscovery() {
  publishDiscoverySensor("temp_outdoor", "Outdoor Temperature",
                         "{{ value_json.temp_c }}", "°C", "temperature", "measurement");
  publishDiscoverySensor("temp_2", "Temperature 2",
                         "{{ value_json.temp2_c }}", "°C", "temperature", "measurement");
  publishDiscoverySensor("temp_board", "Board Temperature",
                         "{{ value_json.board_temp_c }}", "°C", "temperature", "measurement");
  publishDiscoverySensor("humidity", "Humidity",
                         "{{ value_json.humidity }}", "%", "humidity", "measurement");
  publishDiscoverySensor("pressure", "Pressure",
                         "{{ value_json.pressure_hpa }}", "hPa", "atmospheric_pressure",
                         "measurement");
  publishDiscoverySensor("wind_speed", "Wind Speed",
                         "{{ value_json.wind_speed_kph }}", "km/h", "wind_speed",
                         "measurement");
  publishDiscoverySensor("wind_direction", "Wind Direction",
                         "{{ value_json.wind_dir_deg }}", "°", NULL, "measurement");
  publishDiscoverySensor("wind_gust", "Wind Gust",
                         "{{ value_json.wind_gust_kph }}", "km/h", "wind_speed",
                         "measurement");
  publishDiscoverySensor("rain_today", "Rain Today",
                         "{{ value_json.rain_today_mm }}", "mm", "precipitation", "total");
  publishDiscoverySensor("rain_hour", "Rain Last Hour",
                         "{{ value_json.rain_hour_mm }}", "mm", "precipitation",
                         "measurement");
  discoveryPublished = true;
  Serial.println(F("MQTT: Home Assistant discovery published"));
}

static bool mqttReconnect() {
  if (!mqttConfig || !mqttConfig->mqtt.use || !mqttConfig->mqtt.host[0]) {
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  mqttClient.setServer(mqttConfig->mqtt.host, mqttConfig->mqtt.port);
  mqttClient.setBufferSize(1024);

  char clientId[40];
  snprintf(clientId, sizeof(clientId), "weatherstation-%04X",
           (uint16_t)(ESP.getEfuseMac() & 0xFFFF));

  bool ok;
  if (mqttConfig->mqtt.user[0]) {
    ok = mqttClient.connect(clientId, mqttConfig->mqtt.user, mqttConfig->mqtt.password);
  } else {
    ok = mqttClient.connect(clientId);
  }

  if (ok) {
    Serial.print(F("MQTT: connected to "));
    Serial.println(mqttConfig->mqtt.host);
    discoveryPublished = false;
  } else {
    Serial.print(F("MQTT: connect failed rc="));
    Serial.println(mqttClient.state());
  }
  return ok;
}

void mqttSetup(Config *config) {
  mqttConfig = config;
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
  discoveryPublished = false;
  lastMqttPublish = 0;
  lastMqttReconnectAttempt = 0;
  if (!config->mqtt.use || !config->mqtt.host[0]) {
    Serial.println(F("MQTT: disabled"));
    return;
  }
  Serial.print(F("MQTT: broker "));
  Serial.print(config->mqtt.host);
  Serial.print(':');
  Serial.println(config->mqtt.port);
  mqttReconnect();
}

void mqttLoop() {
  if (!mqttConfig || !mqttConfig->mqtt.use || !mqttConfig->mqtt.host[0]) {
    return;
  }

  if (!mqttClient.connected()) {
    unsigned long nowMs = millis();
    if (nowMs - lastMqttReconnectAttempt > 10000UL) {
      lastMqttReconnectAttempt = nowMs;
      mqttReconnect();
    }
    return;
  }

  mqttClient.loop();

  if (!discoveryPublished) {
    publishHomeAssistantDiscovery();
  }

  unsigned long intervalMs =
      (mqttConfig->mqtt.upd8_frequency > 0 ? mqttConfig->mqtt.upd8_frequency : 30) *
      1000UL;
  if (millis() - lastMqttPublish >= intervalMs) {
    mqttPublishTelemetry(mqttConfig);
  }
}

void mqttPublishTelemetry(Config *config) {
  if (!config || !config->mqtt.use || !mqttClient.connected()) {
    return;
  }

  lastMqttPublish = millis();

  JsonDocument doc;
  if (dallasCount > 0) {
    dallas_temperature.requestTemperatures();
  }

  if (dallasCount > 0) {
    float t0 = dallas_temperature.getTempC(dallasAddresses[0]);
    if (dallasTempValidLocal(t0)) {
      doc["temp_c"] = (double)t0;
      doc["temp_f"] = (double)dallas_temperature.getTempF(dallasAddresses[0]);
    }
  }
  if (dallasCount > 1) {
    float t1 = dallas_temperature.getTempC(dallasAddresses[1]);
    if (dallasTempValidLocal(t1)) {
      doc["temp2_c"] = (double)t1;
      doc["temp2_f"] = (double)dallas_temperature.getTempF(dallasAddresses[1]);
    }
  }

  if (BME_ok) {
    doc["board_temp_c"] = (double)tempSensor.readTempC();
    doc["humidity"] = (double)tempSensor.readFloatHumidity();
    doc["pressure_hpa"] = (double)(tempSensor.readFloatPressure() / 100.0f);
  }

  float windKph = weatherMeterKit.getWindSpeed();
  doc["wind_speed_kph"] = (double)windKph;
  doc["wind_dir_deg"] = (int)lroundf(weatherMeterKit.getWindDirection());
  doc["wind_gust_kph"] = (double)maxwind4wu;
  doc["wind_gust_dir_deg"] = maxwinddir4wu;
  doc["rain_today_mm"] = (double)raintoday;
  doc["rain_hour_mm"] = (double)lasthourrain;

  char stateTopic[96];
  mqttBuildTopic(stateTopic, sizeof(stateTopic), "state");

  char payload[768];
  size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) {
    Serial.println(F("MQTT: state payload too large"));
    return;
  }

  if (mqttClient.publish(stateTopic, payload, false)) {
    Serial.print(F("MQTT: published "));
    Serial.println(stateTopic);
  } else {
    Serial.println(F("MQTT: publish failed"));
  }
}
