#include "defines.h"
#include "weathertypes.h"
#include "config.h"
#include "Readings.h"
#include "networks_funcs.h"

NTP *ntp1;
const uint32_t connectTimeoutMs = 10000;

bool start_wifi(Config *config, NTP *ntp, bool restartWiFi = false) {
  if (!(config->wifi.ssid[0] && config->wifi.pwd[0])) {
    return false;
  }
  if (restartWiFi) {
    WiFi.disconnect();
  }
  if (!WiFi.setHostname("Weather Station 1")) {
    Serial.println(F("start_wifi: Hostname not set"));
  }
  if (!config->wifi.usedhcp) {
    if (!WiFi.config(config->wifi.ip, config->wifi.gateway, config->wifi.mask,
                     config->wifi.dns1, config->wifi.dns2)) {
      Serial.println(F("Failed to set static address!"));
    }
  }
  WiFi.begin(config->wifi.ssid, config->wifi.pwd);
  int maxtries = 0;
  while (WiFi.status() != WL_CONNECTED && maxtries < 40) {
    delay(500);
    Serial.print(F("."));
    maxtries++;
  }
  if (maxtries >= 40) {
    return false;
  }
  Serial.print(F("    IP: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("    Gateway: "));
  Serial.println(WiFi.gatewayIP());
  Serial.print(F("    Mask: "));
  Serial.println(WiFi.subnetMask());
  Serial.print(F("    DNS1: "));
  Serial.println(WiFi.dnsIP(0));
  Serial.print(F("    DNS2: "));
  Serial.println(WiFi.dnsIP(1));
  ntp1 = ntp;

  // Australia/Sydney: AEST (standard) UTC+10, AEDT (daylight) UTC+11.
  // DST starts first Sunday in October; ends first Sunday in April.
  ntp->ruleSTD("AEST", First, Sun, Apr, 3, 600);
  ntp->ruleDST("AEDT", First, Sun, Oct, 2, 660);
  ntp->begin();
  delay(2000);
  printLocalTime();

  return true;
}

void printLocalTime() {
  ntp1->update();
  delay(500);
  Serial.println(ntp1->tzName());
  Serial.println(ntp1->formattedTime("%c"));
}


UNITS2REPORT *getRequestedData(UNITS2REPORT *units, JsonDocument doc) {
    // bool lightning;
    if(doc["humidity"].is<bool>()) {
      units->humidity = doc["humidity"];
    }
    if(doc["temperatureI"].is<bool>()) {
      units->temperatureI = doc["temperatureI"];
    }
    if(doc["temperatureX"].is<bool>()) {
      units->temperatureX = doc["temperatureX"];
    }
    if(doc["pressure"].is<bool>()) {
      units->pressure = doc["pressure"];
    }
    if(doc["speed"].is<bool>()) {
      units->speed = doc["speed"];
    }
    if(doc["direction"].is<bool>()) {
      units->direction = doc["direction"];
    }
    if(doc["rain"].is<bool>()) {
      units->rain = doc["rain"];
    }
    if(doc["lightning"].is<bool>()) {
      units->lightning = doc["lightning"];
    }
    if(doc["reportWU"].is<bool>()) {
      units->reportWU = doc["reportWU"];
    }
    if(doc["reportWUonly"].is<bool>()) {
      units->reportWUonly = doc["reportWUonly"];
    }
    if(doc["pretty"].is<bool>()) {
      units->pretty = doc["pretty"];
    }

    return units;

}


void sendUDPMessage(IPAddress ip, port_t port, char *msg, WiFiUDP *udp) {
  if(port < MIN_UDP_PORT || port > MAX_UDP_PORT) {
    return;
  }
  udp->beginPacket(ip, port);
  udp->print(msg);
  if(udp->endPacket()) {
    Serial.print(F("sendUDPMessage: Sent package to port "));
    Serial.print(port);
    Serial.print(F(" at "));
    Serial.println(ip.toString());
  }
}

