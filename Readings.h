#ifndef READINGS_H
#define READINGS_H
#include "defines.h"
#include <WiFi.h>
#include <IPAddress.h>

#define RESPONSES_SUCCESS 0
#define RESPONSES_FAILED 1
#define RESPONSES_DENIED 2
#define RESPONSES_2MANY 3
#define RESPONSES_HERE 4
#define RESPONSES_LISTENER_OK_ACTIVE_OK 5
#define RESPONSES_LISTENER_OK_ACTIVE_BAD 6
#define RESPONSES_LISTENER_ACTIVE_REMOVED 7

#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

#define REPORT_INTERVAL 600
#define LASTHOUR_LENGTH 3600/REPORT_INTERVAL

#define RAINARRAY_LENGTH 60


bool addNewListener(UDP_CLIENTS_TYPE * clients, IPAddress ip, port_t port, UNITS2REPORT *units);
bool addNewListener(UDP_CLIENTS_TYPE * clients, IPAddress ip, port_t port, UNITS2REPORT *units, char source);
void removeListener(UDP_CLIENTS_TYPE * clients, IPAddress ip, port_t port);
void sendUDPMessages(UDP_CLIENTS_TYPE * clients, char *msg, WiFiUDP *udp);
void reloadConfig(const char* , Config* , bool);
void updateWU();
char *createUDPData(char* , UNITS2REPORT*);
char *createUDPData_tabbed(char* , UNITS2REPORT *);
void readHttpResponse(WiFiClient *);
void calibrateWindDirection();
void clearUserInput();
void waitForUserInput();
char *create_url_str(char* param_str);
int readSoil();
int get_rainlasthour();
int get_rain2day();
char *kmh2mph(char* result, float kph);
char *ms2mph(char* result, float ms);
char *c2f(char* result, float c);
char *mm2ft(char* result, float mm);
float mb2in(float mb);
char *dailyRain2in(char* result);
char *hourlyRain2in(char* result);
void reportWUtoUDP(UDP_CLIENTS_TYPE *, char *, WiFiUDP *);
void handleUDPRequests();
int isValidMessage(char *);
#endif
