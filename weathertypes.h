#ifndef WEATHERTYPES_H
#define WEATHERTYPES_H
#include "defines.h"
#include <IPAddress.h>

typedef struct  {
  bool config_ok;
  IPAddress ip;
  unsigned long port;
  char action;
} LISTENER_UPD8_RESULT;

typedef struct 
  {
  char ssid[128] = "";
  char pwd[128] = "";
  IPAddress ip;
  IPAddress mask;
  IPAddress gateway;
  IPAddress dns1;
  IPAddress dns2;
  bool usedhcp = true;
} WIFI_STRUCT;

typedef struct  {
  bool use;
  char device_id[32];
  char device_key[32];
  char api_key[64];
  char url[128];
  char password[32];
  char user_id[128];
  char host[256];
  char endpoint[64];
  char protocol[16];
  int upd8_frequency;
  int port;
} WUNDERGRPOND;


typedef struct  {
  IPAddress ip;
  port_t port;
} LISTENER;

struct UDP_CONFIG {
  bool use;
  port_t port;
  int update_frequency;
  LISTENER listeners[MAX_CLIENTS];
};


typedef struct {
  int noOfssids;
  WIFI_STRUCT wifi;
  WUNDERGRPOND weather_underground;
  UDP_CONFIG udp;
  int elevation;
  float latitude;
  float longitude;
} Config;

class UNITS2REPORT {
  public:
    UNITS2REPORT() {
    this->humidity = true;
    this->temperatureI = true;
    this->temperatureX = true;
    this->pressure = true;
    this->speed = true;
    this->direction = true;
    this->rain = true;
    this->lightning = true;
    this->reportWU = false;
    this->reportWUonly = false;
    this->pretty = false;
    }
    bool humidity;
    bool temperatureI;
    bool temperatureX;
    bool pressure;
    bool speed;
    bool direction;
    bool rain;
    bool lightning;
    bool reportWU;
    bool reportWUonly;
    bool pretty;

    bool commaAfter_humidity() { return (this->temperatureI || this->pressure || this->speed || this->direction || this->rain || this->lightning || this->temperatureX);}
    bool commaAfter_temperatureI() { return (this->pressure || this->speed || this->direction || this->rain || this->lightning || this->temperatureX);}
    bool commaAfter_pressure() { return (this->speed || this->direction || this->rain || this->lightning || this->temperatureX);}
    bool commaAfter_temperatureX() {return (this->speed || this->direction || this->rain || this->lightning);}
    bool commaAfter_speed() { return (this->direction || this->rain || this->lightning);}
    bool commaAfter_direction() { return (this->rain || this->lightning);}
    bool commaAfter_rain() { return this->lightning;}
    bool commaAfter_lightning() { return false;}

};

typedef struct  {
  bool use;
  IPAddress ip;
  port_t port;
  char source;
  UNITS2REPORT units2report;
  bool reportWU;
  bool reportWUonly;
  bool pretty;
} UDP_CLIENTS_TYPE;

#endif