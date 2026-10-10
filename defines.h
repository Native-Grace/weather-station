#ifndef DEFINES_H
#define DEFINES_H

#define FIRMWARE_VERSION "1.2.0"

#define DEBUG false

#define PRINT(x) if(DEBUG) Serial.print(x)
#define PRINTLN(x) if(DEBUG) Serial.println(x)

typedef unsigned int port_t;

#define MAX_CLIENTS 10
#define DEFAULT_UDP_PORT 49999
#define MIN_UDP_PORT 49152
#define MAX_UDP_PORT 65535
#define UDP_SOURCE_CONFIG 'C'
#define UDP_SOURCE_REQUEST 'R'

// #define SPARKFUN_THING_PLUS_C

#ifdef SPARKFUN_THING_PLUS_C
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23

const int sd_cs = 5; //Thing Plus C
#endif

#endif