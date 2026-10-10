#ifndef NETWORK_FUNCS_H
#define NETWORK_FUNCS_H
#include "defines.h"
#include "config.h"
#include <WiFi.h>
#include "time.h"
#include "TimeLib.h"
#include "WiFiUdp.h"
#include "NTP.h"

bool start_wifi(Config *config, NTP *ntp, bool);
void printLocalTime();
UNITS2REPORT *getRequestedData(UNITS2REPORT *, JsonDocument);
void sendUDPMessage(IPAddress ip, port_t port, char *msg, WiFiUDP *udp);

static char VALID_MESSAGE_HEADER[] = "AAAAAAAA";

#endif