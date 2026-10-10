#ifndef CONFIG_H
#define CONFIG_H
#include <ArduinoJson.h>
#include <SD.h>
// #include <string.h>
#include "defines.h"
#include "weathertypes.h"





void checkConfig(Config *config);
void writeConfig(const char *, Config*, bool);
bool editudp(Config*, JsonDocument);
bool editssid(Config*, JsonDocument,  bool*);
bool editWU(Config*, JsonDocument);
bool editListener(Config*, JsonDocument);
bool readFile2buffer(const char *filename, char *buffer, int buffer_length);
bool loadConfiguration(const char*, Config*, bool);

bool editConfiguration(const char*, JsonDocument,  bool*);
void getConfigPart(const char*, JsonDocument, char*, unsigned int, bool* );

#endif

/*****************************************************************************************
Message Formats:
A Valid Message 
starts with 'AAAAAAAAA'
followed by a CHARACTER identfying the message type.
followed by a JSON string unique to the message type
Message Types are   
  '1' UDP Data
  '2' Return UDP Data
  '3' Add sending system as a listener
  '4' A ping request
  '5' Remove sending system as a listener
  '6' Change the configuration
  'C' Lists the various configuration options
  'U' Lists UDP Clients

Message Type: 6
  Add SSID
  {"parameter":"wifi","action":"add","ssid":"testssid", "password": "testpwd"}
  Edit SSID
  {"parameter":"wifi","action":"edit","ssid":"testssid", "password": "testpwd", "usedhcp": true, ", ipaddress": "10.1.2.249", "mask": "255.255.255.0", "gateway", "10.1.2.1", "dns1": "10.1.2.1", "dns2": "10.1.2.1", "restart_wifi" : true}

  Change udp port
  {"parameter":"udp","action":"edit_port","port":12345}
  Change udp usage
  {"parameter":"udp","action":"edit_use","use":false}
  Change udp frequency
  {"parameter":"udp","action":"edit_freq","update_frequency":20}

  Weather Underground url
  {"parameter":"wu","action":"edit_url","host":"weather_undeground.com", "endpoint": "/an/endpoint", "url": "http://weather_undeground.com/an/endpoint", "protocol": "http"}
  Weather Underground use
  {"parameter":"wu","action":"edit_use","use":true}
  Weather Underground device
  {"parameter":"wu","action":"edit_device","device_id":"123456", "device_key": "ABCDEF"}
  Weather Underground user
  {"parameter":"wu","action":"edit_user","user_id":"mikeb", "password": "secret"}

  Add a permanent listener
  {"parameter":"listener","action":"add","ip":"127.0.0.1", "port": 41234}
  Delete a listener
  {"parameter":"listener","action":"delete","ip":"127.0.0.1", "port": 41234}

Message Type: C
  Display part of configuration file (valid parts are wifi, wu ( for weather underground ), udp)
  {"part":"wu"}

Message Type: 3
  Add a listener specifying which readings to send (all arguments are optional the default is shown below)
  {"humidity": true, "temperature": true, "pressure": true, "speed": true, "direction": true, "rain": true, "lightning": true, "reportWU": false, "reportWUonly": false, "pretty": false}
  Remove a listener. If no object included in query then the listener with an ipaddress and port will be removed otherwise
  {"listenerindex": < client index >}


*************************************************************************************/