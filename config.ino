#include "defines.h"
#include "weathertypes.h"
#include "config.h"
#include <WiFi.h>
 enum paras {add ,del, edit, edit_device, edit_port,  edit_url, edit_use, edit_user, edit_freq, bad_action };
 const char* actions[] = { "add", "delete", "edit", "edit_device", "edit_port",  "edit_url", "edit_use", "edit_user", "edit_freq" };

 int findAction (const char* action) {
  for( int i = add; i < bad_action; i++) {
    if (strcmp(action, actions[i]) == 0) {
      return i;
    }
  }
  return bad_action;
 }

 bool readFile2buffer(const char *filename, char *buffer, int buffer_length) {

  // Serial.printf("Reading file: %s\n", path);
  uint16_t i = 0;
  PRINT("readFile2buffer: Opening file: ");
  PRINTLN(filename);
  File file = SD.open(filename, FILE_READ, false);
  if (!file) {
    Serial.println("readFile2buffer: Failed to open file for reading");
    return false;
  }
  PRINT("readFile2buffer: Reading from file: ");
  PRINTLN(filename);

  while (file.available() && i < buffer_length - 1) {
    buffer[i] = file.read();
    i++;
  }
  buffer[i] = 0;
  file.close();

  return true;
}

bool loadConfiguration(const char* filename, Config *config, bool validate_use) {
  char temp[1024];
  char ip[16];
  char mask[16];
  char gateway[16];
  char dns1[16];
  char dns2[16];
  char *buffer;
  buffer = (char *)malloc(4096);

  if(!buffer) {
    Serial.println(F("loadConfiguration: No enough memory"));
    return false;
  }

  if(!readFile2buffer(filename, buffer, 4096)){
    Serial.println(F("loadConfiguration: File read Failed"));
    return false;
  }

  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, buffer);
  free(buffer);
  
  if (error)
    {
      Serial.println(F("loadConfiguration: Failed to deserialize the file"));
      return false;
    }
  // Copy values from the JsonDocument to the Config
  config->elevation = doc["elevation"];
  config->latitude = doc["latitude"];
  config->longitude = doc["longitude"];

  if(doc["wifi"].is<JsonObject>()) {
    config->wifi.usedhcp = doc["wifi"]["usedhcp"];
    strncpy(config->wifi.pwd, doc["wifi"]["password"] | "", sizeof(config->wifi.pwd));
    strncpy(config->wifi.ssid, doc["wifi"]["ssid"] | "", sizeof(config->wifi.ssid));
    strncpy(ip, doc["wifi"]["ip"] | "", sizeof(ip));
    if (ip[0]) {
      config->wifi.ip.fromString(ip);
    }
    strncpy(mask, doc["wifi"]["mask"] | "", sizeof(mask));
    if (mask[0]) {
      config->wifi.mask.fromString(mask);
    }
    strncpy(gateway, doc["wifi"]["gateway"] | "", sizeof(gateway));
    if (gateway[0]) {
      config->wifi.gateway.fromString(gateway);
    }
    strncpy(dns1, doc["wifi"]["dns1"] | "", sizeof(dns1));
    if (dns1[0]) {
      config->wifi.dns1.fromString(dns1);
    }
    strncpy(dns2, doc["wifi"]["dns2"] | "", sizeof(dns2));
    if (dns2[0]) {
      config->wifi.dns2.fromString(dns2);
    }
  } else {
    config->wifi.ssid[0] = 0;
    config->wifi.pwd[0] = 0;
  }
 
  config->weather_underground.use = doc["weather_underground"]["use"];
  strncpy(config->weather_underground.device_id, doc["weather_underground"]["device_id"] | "", sizeof(config->weather_underground.device_id));
  strncpy(config->weather_underground.device_key, doc["weather_underground"]["device_key"] | "", sizeof(config->weather_underground.device_key));
  strncpy(config->weather_underground.api_key, doc["weather_underground"]["api_key"] | "", sizeof(config->weather_underground.api_key));
  strncpy(config->weather_underground.url, doc["weather_underground"]["url"] | "", sizeof(config->weather_underground.url));
  strncpy(config->weather_underground.password, doc["weather_underground"]["password"] | "", sizeof(config->weather_underground.password));
  strncpy(config->weather_underground.user_id, doc["weather_underground"]["user_id"] | "", sizeof(config->weather_underground.user_id));
  strncpy(config->weather_underground.host, doc["weather_underground"]["host"] | "", sizeof(config->weather_underground.host));

  strncpy(temp, doc["weather_underground"]["endpoint"] | "", sizeof(config->weather_underground.endpoint));
  if (strlen(temp) > 0) {
    if(temp[0] == '/') {
      strncpy(config->weather_underground.endpoint, temp, sizeof(config->weather_underground.endpoint));
    } else {
      strncpy(config->weather_underground.endpoint, "/", 1);
      strncat(config->weather_underground.endpoint, temp, strlen(temp));
    }
  } else {
    config->weather_underground.endpoint[0] = 0;
  }

  strncpy(config->weather_underground.protocol, doc["weather_underground"]["protocol"] | "", sizeof(config->weather_underground.protocol));
  if (doc["weather_underground"]["upd8_frequency"].is<signed int>()) {
    config->weather_underground.upd8_frequency = doc["weather_underground"]["upd8_frequency"].as<signed int>();
  } else {
    Serial.println("weather_underground.upd8_frequency is not valid");
    config->weather_underground.upd8_frequency = 0;
  }
  
  char *ptr2colon;
  ptr2colon = strstr(config->weather_underground.host, ":");
  if (ptr2colon) {
    *ptr2colon = '\0';
    ptr2colon++;
    config->weather_underground.port = atoi(ptr2colon);
  } else {
    if(strcmp(config->weather_underground.protocol, "http") == 0 ) {
      config->weather_underground.port = 80;
    } else {
      config->weather_underground.port = 443;
    }
  }

  config->udp.use = doc["udp"]["use"];
  if(doc["udp"]["update_frequency"].is<unsigned long>()) {
    config->udp.update_frequency = doc["udp"]["update_frequency"].as<unsigned long>();
  } else {
    config->udp.update_frequency = 10;
  }

  // MQTT (Home Assistant)
  config->mqtt.use = false;
  config->mqtt.host[0] = 0;
  config->mqtt.port = 1883;
  config->mqtt.user[0] = 0;
  config->mqtt.password[0] = 0;
  strncpy(config->mqtt.base_topic, "weatherstation", sizeof(config->mqtt.base_topic));
  strncpy(config->mqtt.discovery_prefix, "homeassistant",
          sizeof(config->mqtt.discovery_prefix));
  config->mqtt.upd8_frequency = 30;
  if (doc["mqtt"].is<JsonObject>()) {
    config->mqtt.use = doc["mqtt"]["use"] | false;
    strncpy(config->mqtt.host, doc["mqtt"]["host"] | "", sizeof(config->mqtt.host));
    if (doc["mqtt"]["port"].is<signed int>()) {
      config->mqtt.port = doc["mqtt"]["port"].as<signed int>();
    }
    strncpy(config->mqtt.user, doc["mqtt"]["user"] | "", sizeof(config->mqtt.user));
    strncpy(config->mqtt.password, doc["mqtt"]["password"] | "",
            sizeof(config->mqtt.password));
    if (doc["mqtt"]["base_topic"].is<const char *>()) {
      strncpy(config->mqtt.base_topic, doc["mqtt"]["base_topic"] | "weatherstation",
              sizeof(config->mqtt.base_topic));
    }
    if (doc["mqtt"]["discovery_prefix"].is<const char *>()) {
      strncpy(config->mqtt.discovery_prefix,
              doc["mqtt"]["discovery_prefix"] | "homeassistant",
              sizeof(config->mqtt.discovery_prefix));
    }
    if (doc["mqtt"]["upd8_frequency"].is<signed int>()) {
      config->mqtt.upd8_frequency = doc["mqtt"]["upd8_frequency"].as<signed int>();
    }
    if (config->mqtt.use && !config->mqtt.host[0]) {
      config->mqtt.use = false;
      Serial.println(F("checkConfig: MQTT host empty"));
    }
  }
  unsigned long p = doc["udp"]["port"].as<unsigned long>();
  if (p >= MIN_UDP_PORT && p <= MAX_UDP_PORT) {
    config->udp.port = (port_t) p;
  } else {
    config->udp.port = DEFAULT_UDP_PORT;
  }
  if(doc["udp"]["listeners"].is<JsonArray>()) {
    char ipstr[16];
    
    int maxindex = min(MAX_CLIENTS, (int) doc["udp"]["listeners"].size() );
    for ( int i = 0; i< MAX_CLIENTS ; i++) {
      p = doc["udp"]["listeners"][i]["port"].as<unsigned long>();
      if (i < maxindex && p >= MIN_UDP_PORT && p <= MAX_UDP_PORT) {
        strncpy(ipstr, doc["udp"]["listeners"][i]["ip"] | "", sizeof(ipstr));
        config->udp.listeners[i].ip.fromString(ipstr);
        config->udp.listeners[i].port = (port_t) p;
      } else {
        config->udp.listeners[i].port = 0;
      }
    }
  }

  if(validate_use) {
    checkConfig(config);
  }
  return true;
}

void checkConfig(Config *config) {
  if(!config->wifi.usedhcp) {
    if( !(config->wifi.ip && config->wifi.mask && config->wifi.gateway )) {
      if(!config->wifi.ip) {
        Serial.print("checkConfig: Invalid Wifi Ip address:- ");
        Serial.println(config->wifi.ip.toString());
      }
      if(!config->wifi.mask) {
        Serial.print("checkConfig: Invalid wifi Mask address");
      }
      if(!config->wifi.gateway) {
        Serial.print("checkConfig: Invalid wifi gateway address");
      }
      config->wifi.usedhcp = true;
    }
  }
  if (strlen(config->weather_underground.device_id) == 0 ) {
    config->weather_underground.use = false;
    Serial.println(F("checkConfig: Weather Underground device_id empty"));
  }
  if (strlen(config->weather_underground.device_key) == 0 ) {
    config->weather_underground.use = false;
    Serial.println(F("checkConfig: Weather Underground device_key empty"));
  }
  if (strlen(config->weather_underground.host) == 0 ) {
    config->weather_underground.use = false;
    Serial.println(F("checkConfig: Weather Underground host is empty"));
  }
  if (strlen(config->weather_underground.endpoint) == 0 ) {
    config->weather_underground.use = false;
    Serial.println(F("loadConfiguration: Weather Underground endpoint empty"));
  }
  if (strlen(config->weather_underground.protocol) == 0  ) {
    config->weather_underground.use = false;
    Serial.println(F("loadConfiguration: Weather Underground protocol empty"));
  } else {
    if(!(strcmp(config->weather_underground.protocol, "http") == 0 || strcmp(config->weather_underground.protocol, "https") == 0 ) ) {
      config->weather_underground.use = false;
    }
  }
  if ((config->weather_underground.port == 0)) {
    config->weather_underground.use = false;
    Serial.println("loadConfiguration: Weather Underground port = 0");
  }

  if ((config->weather_underground.upd8_frequency == 0)) {
    config->weather_underground.use = false;
    Serial.println(F("loadConfiguration: Weather Underground update frequency = 0"));
  }
  // if (strlen(config->weather_underground.api_key) == 0 ) {
  //   config->weather_underground.use = false;
  //   Serial.println("loadConfiguration: Weather Underground api_key empty");
  // }
  // if (strlen(config->weather_underground.url) == 0 ) {
  //   config->weather_underground.use = false;
  // }
  // if (strlen(config->weather_underground.rassword) == 0 ) {
  //   config->weather_underground.use = false;
  // }
  // if (strlen(config->weather_underground.user_id) == 0 ) {
  //   config->weather_underground.use = false;
  // }

}

bool editudp(Config *config, JsonDocument doc) {
  char action[128];
  port_t port;
  bool useudp;
  unsigned long freq;
  if (doc["action"].is<std::string>()) {
    strncpy(action, doc["action"] | "", sizeof(action) );
    switch(findAction(action)) {
      case edit_port:
        if(!doc["port"].is<unsigned long>()) {
          Serial.println(F("editudp: Missing port field in UDP update"));
          return false;
        }
        port = doc["port"];
        config->udp.port = port;
        return true;
      case edit_use:
        if(!doc["use"].is<bool>()) {
          Serial.println(F("editudp: Missing use field in UDP update"));
          return false;
        }
        useudp = doc["use"].as<bool>();
        config->udp.use = useudp;
        return true;
      case edit_freq:
        if(!doc["update_frequency"].is<unsigned long>()) {
          Serial.println(F("editudp: Missing use field in UDP update"));
          return false;
        }
        freq = doc["update_frequency"].as<unsigned long>();
        config->udp.update_frequency = freq;
        return true;
      case bad_action:
        Serial.print(F("editudp: Unknown Action in UDP update: "));
        Serial.println(action);
    }
  }
  return false;
}

bool editWU(Config *config, JsonDocument doc) {
  char action[128];

  PRINT("editWU: start");

  if (doc["action"].is<std::string>()) {
    strncpy(action, doc["action"] | "", sizeof(action) );

    PRINT("editWU: action: ");
    PRINTLN(action);

    switch(findAction(action)) {
      case edit_url:
        if (doc["host"].is<std::string>() && doc["protocol"].is<std::string>() && doc["url"].is<std::string>() && doc["endpoint"].is<std::string>()) {
          strncpy(config->weather_underground.host, doc["host"] | "" , sizeof(config->weather_underground.host));
          strncpy(config->weather_underground.protocol, doc["protocol"] | "" , sizeof(config->weather_underground.protocol));
          strncpy(config->weather_underground.url, doc["url"] | "" , sizeof(config->weather_underground.url));
          strncpy(config->weather_underground.endpoint, doc["endpoint"] | "" , sizeof(config->weather_underground.endpoint));
          return true;
        } else {
          Serial.println(F("editWU: Missing data in Weather Underground URL update"));
        }
        return true;
      case edit_use:
        if (doc["use"].is<bool>()) {
          config->weather_underground.use = doc["use"].as<bool>();
          return true;
        } else {
          Serial.println(F("editWU: Missing 'use' field in Weather Underground use update"));
        }
        return true;
      case edit_device:
        if (doc["device_id"].is<std::string>() && doc["device_key"].is<std::string>()) {
          strncpy(config->weather_underground.device_id, doc["device_id"] | "" , sizeof(config->weather_underground.device_id));
          strncpy(config->weather_underground.device_key, doc["device_key"] | "" , sizeof(config->weather_underground.device_key));
          return true;
        } else {
          Serial.println(F("editWU: Missing use field in Weather Underground use update"));
        }
        return true;
      case edit_user:

        if (doc["user_id"].is<std::string>() && doc["password"].is<std::string>()) {
          strncpy(config->weather_underground.user_id, doc["user_id"] | "" , sizeof(config->weather_underground.user_id));
          PRINT("editWU: oc[\"user_id\"]: ");
          PRINTLN(config->weather_underground.user_id);
          strncpy(config->weather_underground.password, doc["password"] | "" , sizeof(config->weather_underground.password));
          PRINT("editWU: oc[\"password\"]: ");
          PRINTLN(config->weather_underground.password);
          return true;
        } else {
          Serial.println(F("editWU: Missing user_id or password field in Weather Underground use update"));
        }
        return true;
      case bad_action:
        Serial.print(F("editWU: Unknown action in Weather Underground update: "));
        Serial.println(action);
    }
  } else {
    Serial.println(F("editWU: Missing action field in Weather Underground update"));
  }
  return false;
}

bool editMQTT(Config *config, JsonDocument doc) {
  // {"parameter":"mqtt","host":"192.168.2.10","port":1883,"use":true,...}
  if (!doc["host"].is<std::string>() || strlen(doc["host"] | "") == 0) {
    Serial.println(F("editMQTT: Missing host in MQTT update"));
    return false;
  }
  strncpy(config->mqtt.host, doc["host"] | "", sizeof(config->mqtt.host));
  if (doc["port"].is<signed int>()) {
    config->mqtt.port = doc["port"].as<signed int>();
  }
  if (doc["use"].is<bool>()) {
    config->mqtt.use = doc["use"].as<bool>();
  } else {
    config->mqtt.use = true;
  }
  if (doc["user"].is<std::string>()) {
    strncpy(config->mqtt.user, doc["user"] | "", sizeof(config->mqtt.user));
  }
  if (doc["password"].is<std::string>()) {
    strncpy(config->mqtt.password, doc["password"] | "", sizeof(config->mqtt.password));
  }
  if (doc["base_topic"].is<std::string>()) {
    strncpy(config->mqtt.base_topic, doc["base_topic"] | "weatherstation",
            sizeof(config->mqtt.base_topic));
  }
  if (doc["discovery_prefix"].is<std::string>()) {
    strncpy(config->mqtt.discovery_prefix, doc["discovery_prefix"] | "homeassistant",
            sizeof(config->mqtt.discovery_prefix));
  }
  if (doc["upd8_frequency"].is<signed int>()) {
    config->mqtt.upd8_frequency = doc["upd8_frequency"].as<signed int>();
  }
  return true;
}

bool editListener(Config *oldconfig, JsonDocument doc) {
  char action[128];
  char ipstr[16];
  IPAddress ip;
  port_t port;
  // char parameter[128];

  if (doc["action"].is<std::string>()) {
    strncpy(action, doc["action"] | "", sizeof(action) );
    if (!(doc["ip"].is<std::string>() && doc["port"].is<unsigned long>())) {
      Serial.println(F("editListener: Missing ip or port in Listener edit"));
      return false;
    }

    PRINT(F("editListener: action = "));
    PRINTLN(action);

    switch (findAction(action)) {
      case add:
        {
          int i;
          unsigned long p = doc["port"].as<unsigned long>();
          if (p < MIN_UDP_PORT || p > MAX_UDP_PORT) {
            Serial.print(F("editListener: Invalid port number: "));
            Serial.println(p);
            return false;
          }
          for(i= 0; i < MAX_CLIENTS; i++) {
            if (oldconfig->udp.listeners[i].port < MIN_UDP_PORT){
              strncpy(ipstr, doc["ip"] | "", sizeof(ipstr));
              ip.fromString(ipstr);
              oldconfig->udp.listeners[i].port = (port_t)p;
              oldconfig->udp.listeners[i].ip = ip;
              break;
            }
          }
          if ( i >= MAX_CLIENTS) {
            Serial.print(F("editListener: Unable to add listener:"));
            return false;
          }
          return true;
        }
      case del:
        {
          strncpy(ipstr, doc["ip"] | "", sizeof(ipstr));
          ip.fromString(ipstr);
          port = (port_t)doc["port"].as<unsigned short>();
          int i;
          for( i = 0; i < MAX_CLIENTS; i++) {
            if (oldconfig->udp.listeners[i].port == port && oldconfig->udp.listeners[i].ip == ip){
              oldconfig->udp.listeners[i].port = 0;
              break;
            }
          }
          if ( i == MAX_CLIENTS) {
            Serial.print(F("editListener: Unable to Delete listener:"));
            return false;
          }
          return true;
        }
      case bad_action:
        Serial.print(F("editListener: Unknown action in Listener update: "));
        Serial.print(action);
    }
  } else {
    Serial.println(F("editListener: Missing action field in listener update"));
  }
  return false;
}

bool editssid(Config *config, JsonDocument doc, bool *restartWiFi) {
  char action[128];
  char ssid[128];
  char pwd[128];
  char ip[16];
  char mask[16];
  char gateway[16];
  char dns1[16];
  char dns2[16];
  bool restart_requested = false;
  bool restart_request_specified = false;
  Config oldconfig;
  if (doc["action"].is<std::string>() && doc["ssid"].is<std::string>()) {
    strncpy(action, doc["action"] | "", sizeof(action) );
    strncpy(ssid, doc["ssid"] | "", sizeof(ssid) );
    if (strlen(ssid) == 0) {
      Serial.println(F("editssid: Invalid SSID in SSID update"));
      return false;
    }
    if (!doc["password"].is<std::string>()) {
      Serial.println(F("editssid: Missing password in SSID edit"));
      return false;
    }
    strncpy(pwd, doc["password"] | "", sizeof(pwd) );
    if (strlen(pwd) == 0) {
      Serial.println(F("editssid: Invalid password in SSID update"));
      return false;
    }
    memcpy(&oldconfig, config, sizeof(oldconfig));
    *restartWiFi = false;
    strncpy(config->wifi.pwd, pwd, sizeof(config->wifi.pwd));
    *restartWiFi = *restartWiFi || (strcmp(oldconfig.wifi.pwd, config->wifi.pwd) != 0);
    strncpy(config->wifi.ssid, ssid, sizeof(config->wifi.ssid));
    *restartWiFi = *restartWiFi || (strcmp(oldconfig.wifi.ssid, config->wifi.ssid) != 0);

    if(doc["restart_wifi"].is<bool>()) {
      restart_requested = doc["restart_wifi"].as<bool>();
      restart_request_specified = true;
    }

    switch(findAction(action)) {
      case edit:
          if(doc["usedhcp"].is<bool>()) {
            config->wifi.usedhcp = doc["usedhcp"].as<bool>();
            *restartWiFi = *restartWiFi || (oldconfig.wifi.usedhcp == config->wifi.usedhcp);
          }
          if(doc["ipaddress"].is<std::string>()){
            strncpy(ip, doc["ipaddress"] | "", sizeof(ip));
            if (ip[0]) {
              config->wifi.ip.fromString(ip);
              *restartWiFi = *restartWiFi || (oldconfig.wifi.ip == config->wifi.ip);
            }
          }

          if(doc["mask"].is<std::string>()){
            strncpy(mask, doc["mask"] | "", sizeof(mask));
            if (mask[0]) {
              config->wifi.mask.fromString(mask);
              *restartWiFi = *restartWiFi || (oldconfig.wifi.mask == config->wifi.mask);
            }
          } else {
            Serial.println("editssid: Bad mask");
          }

          if(doc["gateway"].is<std::string>()){
            strncpy(gateway, doc["gateway"] | "", sizeof(gateway));
            if (gateway[0]) {
              config->wifi.gateway.fromString(gateway);
              *restartWiFi = *restartWiFi || (oldconfig.wifi.gateway == config->wifi.gateway);
            }
          } else {
            Serial.println("editssid: Bad gateway");
          }

          if(doc["dns1"].is<std::string>()){
            strncpy(dns1, doc["dns1"] | "", sizeof(dns1));
            if (dns1[0]) {
              config->wifi.dns1.fromString(dns1);
              *restartWiFi = *restartWiFi || (oldconfig.wifi.dns1 == config->wifi.dns1);
            }
          } else {
            Serial.println("editssid: Bad dns1");
          }

          if(doc["dns2"].is<std::string>()){
            strncpy(dns2, doc["wifi"]["dns2"] | "", sizeof(dns2));
            if (dns2[0]) {
              config->wifi.dns2.fromString(dns2);
              *restartWiFi = *restartWiFi || (oldconfig.wifi.dns2 == config->wifi.dns2);
            }
          } else {
            Serial.println("editssid: Bad dns2");
          }

          if(restart_request_specified) {
            *restartWiFi = restart_requested;
          }
          return true;
          break;
      case bad_action:
        Serial.print(F("editssid: Unknow action in wifi update: "));
        Serial.println(action);
    }
  } else {
    Serial.println(F("editssid: Missing action or ssid field in wifi update"));
  }
  return false;
}


void writeConfig(const char *filename, Config *config, bool toSerial = false) {
  // Serial.print(F("writeConfig: About to save new config to "));
  // Serial.println(filename);

  JsonDocument doc;
  JsonDocument clientdoc;
  char *buffer;



  doc["elevation"] = config->elevation;
  doc["latitude"] = config->latitude;
  doc["longitude"] = config->longitude;

 
  // Serial.println(F("writeConfig: About to setup udp."));
  
  JsonObject udp = doc["udp"].to<JsonObject>();
  udp["port"] = config->udp.port;
  udp["use"] = config->udp.use;
  udp["update_frequency"] = config->udp.update_frequency;
  JsonArray listeners = udp["listeners"].to<JsonArray>();
  

  // Serial.println(F("writeConfig: About to setup udp listeners."));

  for(int i = 0; i < MAX_CLIENTS; i++) {
    if (config->udp.listeners[i].port >= MIN_UDP_PORT) {
      if(listeners.add<JsonObject>()) {
        listeners[i]["ip"] = config->udp.listeners[i].ip.toString();
        listeners[i]["port"] = config->udp.listeners[i].port;
      }
    }
  }

  // Serial.println(F("writeConfig: About to setup weather underground."));
  JsonObject weather_underground = doc["weather_underground"].to<JsonObject>();
  weather_underground["protocol"] = config->weather_underground.protocol;
  weather_underground["endpoint"] = config->weather_underground.endpoint;
  weather_underground["host"] = config->weather_underground.host;
  weather_underground["user_id"] = config->weather_underground.user_id;
  weather_underground["password"] = config->weather_underground.password;
  weather_underground["url"] = config->weather_underground.url;
  weather_underground["api_key"] = config->weather_underground.api_key;
  weather_underground["device_key"] = config->weather_underground.device_key;
  weather_underground["device_id"] = config->weather_underground.device_id;
  weather_underground["use"] = config->weather_underground.use;
  weather_underground["upd8_frequency"] = config->weather_underground.upd8_frequency;



  JsonObject mqtt = doc["mqtt"].to<JsonObject>();
  mqtt["use"] = config->mqtt.use;
  mqtt["host"] = config->mqtt.host;
  mqtt["port"] = config->mqtt.port;
  mqtt["user"] = config->mqtt.user;
  mqtt["password"] = config->mqtt.password;
  mqtt["base_topic"] = config->mqtt.base_topic;
  mqtt["discovery_prefix"] = config->mqtt.discovery_prefix;
  mqtt["upd8_frequency"] = config->mqtt.upd8_frequency;

  // Serial.println(F("writeConfig: About to setup wifi."));
  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["ssid"] = config->wifi.ssid;
  wifi["password"] = config->wifi.pwd;
  wifi["ip"] = config->wifi.ip.toString();
  wifi["mask"] = config->wifi.mask.toString();
  wifi["gateway"] = config->wifi.gateway.toString();
  wifi["dns1"] = config->wifi.dns1.toString();
  wifi["dns2"] = config->wifi.dns2.toString();
  wifi["usedhcp"] = config->wifi.usedhcp;

  doc.shrinkToFit();  // optional
  if (!toSerial) {
    // Serial.print("About to delete ");
    // Serial.println(filename);
    SD.remove(filename);
    // Serial.print("Deleted ");
    // Serial.println(filename);
    // Serial.print("About to open ");
    // Serial.println(filename);
    File file = SD.open(filename, FILE_WRITE);

    if (file) {
      // Serial.print("Opened ");
      // Serial.println(filename);
      buffer = (char *)malloc(4096);
      serializeJsonPretty(doc, buffer, 4095);
      file.print(buffer);
      delay(500);
      free(buffer);
      file.close();
      // #ifdef DEBUG
      //   Serial.print(F("writeConfig: Number of characters writen to file: "));
      //   Serial.println(charsout);
      // #endif
    } else {
      Serial.print("Could not open ");
      Serial.println(filename);

    }
  } else {
    serializeJsonPretty(doc, Serial);
    Serial.println();
  }
}

void printConfig(Config *config) {
  Serial.print("{\n" );
  Serial.print("    \"elevation\": ");
  Serial.print(config->elevation);
  Serial.println(",");
  Serial.print("    \"latitude\": ");
  Serial.print(config->latitude);
  Serial.println(",");
  Serial.print("    \"longitude\": ");
  Serial.print(config->longitude);
  Serial.println(",");
  Serial.println("    \"udp\": {");
  Serial.print("        \"port\": ");
  Serial.print(config->udp.port);
  Serial.println(",");
  Serial.print("        \"use\": ");
  Serial.print(config->udp.use ? "true" : "false");
  Serial.println(",");
  Serial.println("        \"listeners\" : [");

  for(int i = 0; i < MAX_CLIENTS; i++) {
    if (config->udp.listeners[i].port){
      Serial.println("            {");
      Serial.print("             \"ip\": \"");
      Serial.print(config->udp.listeners[i].ip.toString());
      Serial.println("\",");
      Serial.print("             \"port\": ");
      Serial.print(config->udp.listeners[i].port);
      Serial.print("            }");
      Serial.println((i < MAX_CLIENTS-1) ? "," : "");
    }
  }
  Serial.println("         ]");
  Serial.println("    },");
  Serial.println("    \"wifi\": {");
  Serial.print  ("        \"ssid\": \"");
  Serial.print  (config->wifi.ssid);
  Serial.println("\",");
  Serial.print  ("        \"pwd\": \"");
  Serial.print  (config->wifi.pwd);
  Serial.println("\",");
  Serial.print  ("        \"ip\": \"");
  Serial.print  (config->wifi.ip.toString());
  Serial.println("\",");
  Serial.print  ("        \"mask\": \"");
  Serial.print  (config->wifi.mask.toString());
  Serial.println("\",");
  Serial.print  ("        \"gateway\": \"");
  Serial.print  (config->wifi.gateway.toString());
  Serial.println("\",");
  Serial.print  ("        \"dns1\": \"");
  Serial.print  (config->wifi.dns1.toString());
  Serial.println("\",");
  Serial.print  ("        \"dns2\": \"");
  Serial.print  (config->wifi.dns2.toString());
  Serial.println("\",");
  Serial.print  ("        \"usedhcp\": ");
  Serial.print  (config->wifi.usedhcp ? "true" : "false");
  Serial.println("");
  Serial.println("       },");
  Serial.println("       \"weather_underground\": {");
  Serial.print  ("            \"protocol\": \"");
  Serial.print  (config->weather_underground.protocol);
  Serial.println("\",");
  Serial.print  ("            \"endpoint\": \"");
  Serial.print  (config->weather_underground.endpoint);
  Serial.println("\",");
  Serial.print  ("            \"host\": \"");
  Serial.print  (config->weather_underground.host);
  Serial.println("\",");
  Serial.print  ("            \"user_id\": \"");
  Serial.print  (config->weather_underground.user_id);
  Serial.println("\",");
  Serial.print  ("            \"password\": \"");
  Serial.print  (config->weather_underground.password);
  Serial.println("\",");
  Serial.print  ("            \"url\": \"");
  Serial.print  (config->weather_underground.url);
  Serial.println("\",");
  Serial.print  ("            \"api_key\": \"");
  Serial.print  (config->weather_underground.api_key);
  Serial.println("\",");
  Serial.print  ("            \"device_key\": \"");
  Serial.print  (config->weather_underground.device_key);
  Serial.println("\",");
  Serial.print  ("            \"device_id\": \"");
  Serial.print  (config->weather_underground.device_id);
  Serial.println("\",");
  Serial.print  ("            \"upd8_frequency\": ");
  Serial.print  (config->weather_underground.upd8_frequency);
  Serial.println(",");
  Serial.print  ("            \"use\": ");
  Serial.println(config->weather_underground.use ? "true" : "false");
    Serial.print  ("            \"port\": ");
  Serial.println(config->weather_underground.port);
  Serial.println("        }");
  Serial.println("}");

}


bool editConfiguration(const char* filename, JsonDocument doc, bool *restartWiFi) {
  PRINTLN("editConfiguration: start");
  Config oldconfig;
  char parameter[128];
  if(!doc["parameter"].is<std::string>()) {
    Serial.println(F("editConfiguration: Parameter is not a string"));
    return false;
  } 
  
  strncpy(parameter, doc["parameter"] | "", sizeof(parameter));

  PRINTLN("editConfiguration: about to loadConfiguration wifi.");
  loadConfiguration(filename, &oldconfig, false);
  if(strcmp(parameter, "wifi") == 0) {
    PRINTLN("editConfiguration: about to EDIT wifi.");
    if (editssid(&oldconfig, doc, restartWiFi)) {
      PRINTLN("editConfiguration: about to write new configuration.");
      writeConfig(filename, &oldconfig);
      return true;
    } else {
      Serial.println(F("editConfiguration: Edit UDP Failed"));
    }
  } else if(strcmp(parameter, "udp") == 0) {
    if (editudp(&oldconfig, doc)) {
      writeConfig(filename, &oldconfig);
      return true;
    }  else {
      Serial.println(F("editConfiguration: Edit UDP Failed"));
    } 
  }  else if(strcmp(parameter, "wu") == 0) {
    if (editWU(&oldconfig, doc)) {
      writeConfig(filename, &oldconfig);
      return true;
    }  else {
      Serial.println(F("editConfiguration: Edit UDP Failed"));
    } 
  }  else if(strcmp(parameter, "mqtt") == 0) {
    if (editMQTT(&oldconfig, doc)) {
      writeConfig(filename, &oldconfig);
      return true;
    }  else {
      Serial.println(F("editConfiguration: Edit MQTT Failed"));
    }
  }  else if(strcmp(parameter, "listener") == 0) {
    if (editListener(&oldconfig, doc)) {
      writeConfig(filename, &oldconfig);
      return true;
    }  else {
      Serial.println(F("editConfiguration: Edit UDP Failed"));
    }
  }  else {
    Serial.print("editConfiguration: Unknown Parameter to Edit: ");
    Serial.println(parameter);
  }
  return false;
}

void getConfigPart(const char* filename, JsonDocument docin, char* buffer, unsigned int sizeofbuffer, bool* success ) {
  char part[128];
  char *temp;
  int templen;
  if(!docin["part"].is<std::string>()) {
    Serial.println(F("getConfigPart: Missing part name"));
    *success = false;
    return;
  }
  strncpy(part, docin["part"] | "" , sizeof(part));
  if(strlen(part) == 0) {
    Serial.println(F("getConfigPart: Invalid part name"));
    *success = false;
    return;
  }
  PRINT(F("getConfigPart: part:- "));
  PRINTLN(part);
  temp = (char*)malloc(4096);
  if(!temp) {
    return;
  }
  if(!readFile2buffer(filename, temp, 4096)){
    Serial.println(F("getConfigPart: Card Mount Failed"));
    free(temp);
    return;
  }
  
  JsonDocument doc;

  JsonDocument wifi;
 // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, temp);
  
  if (error)
    {
      Serial.println(F("getConfigPart: Failed to deserialize file"));
      free(temp);
      return;
    }

  
  if (strcmp(part, "wifi") == 0) {
    if(!doc[part].is<JsonObject>()) {
      Serial.println("getConfigPart: wifi element missing or of wrong type");
      *success = false;
      free(temp);
      return;
    }

    JsonDocument currentDoc;

    currentDoc["ssid"] = WiFi.SSID();
    currentDoc["rssi"] = WiFi.RSSI();
    currentDoc["ip"] = WiFi.localIP();
    currentDoc["gateway"] = WiFi.gatewayIP();
    currentDoc["mask"] = WiFi.subnetMask();
    // JsonObject wifi = doc[part].as<JsonObject>();
    wifi["current"] = currentDoc.as<JsonObject>();
    wifi["settings"] = doc[part].as<JsonObject>();
    serializeJson(wifi, temp, 4096);
    templen = strlen(temp);
    temp[templen] = '\n';
    temp[min(templen+1,4095)] = 0;
    if (strlen(temp) < sizeofbuffer-1) {
      *success = true;
      strncpy(buffer,temp, sizeofbuffer-1);
      strcat(buffer,"\n");
      free(temp);
      return;
    }
    *success = false;
    return;
  } else if (strcmp(part, "wu") == 0) {
    if(!doc["weather_underground"].is<JsonObject>()) {
      Serial.println(F("getConfigPart: weather_underground element missing or of wrong type"));
      *success = false;
      free(temp);
      return;
    }
    doc["weather_underground"]["password"] = "****************";
    JsonObject wu = doc["weather_underground"].as<JsonObject>();
    serializeJson(wu, temp, 4096);
    templen = strlen(temp);
    temp[templen] = '\n';
    temp[min(templen+1, 4095)] = 0;
    if (strlen(temp) < sizeofbuffer) {
      *success = true;
      strncpy(buffer,temp, sizeofbuffer);
      free(temp);
      return;
    }
    *success = false;
    free(temp);
    return;
  } else if (strcmp(part, "mqtt") == 0) {
    if(!doc["mqtt"].is<JsonObject>()) {
      Serial.println(F("getConfigPart: mqtt element missing or of wrong type"));
      *success = false;
      free(temp);
      return;
    }
    doc["mqtt"]["password"] = "****************";
    JsonObject mqtt = doc["mqtt"].as<JsonObject>();
    serializeJson(mqtt, temp, 4096);
    templen = strlen(temp);
    temp[templen] = '\n';
    temp[min(templen+1,4095)] = 0;
    if (strlen(temp) < sizeofbuffer) {
      *success = true;
      strncpy(buffer,temp, sizeofbuffer);
      free(temp);
      return;
    }
    *success = false;
    free(temp);
    return;
  } else if (strcmp(part, "udp") == 0) {
    if(!doc[part].is<JsonObject>()) {
      Serial.println(F("getConfigPart: udp element missing or of wrong type"));
      *success = false;
      free(temp);
      return;
    }
    JsonObject udp = doc[part].as<JsonObject>();
    serializeJson(udp, temp, 4096);
    templen = strlen(temp);
    temp[templen] = '\n';
    temp[min(templen+1, 4095)] = 0;
    if (strlen(temp) < sizeofbuffer) {
      *success = true;
      strncpy(buffer,temp, sizeofbuffer);
      free(temp);
      return;
    }
    *success = false;
    free(temp);
    return;
  } else {
    Serial.println(F("getConfigPart: unknown part"));
  }
  free(temp);
  *success = false;
}

