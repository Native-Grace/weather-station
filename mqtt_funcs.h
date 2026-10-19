#ifndef MQTT_FUNCS_H
#define MQTT_FUNCS_H

#include "config.h"

void mqttSetup(Config *config);
void mqttLoop();
void mqttPublishTelemetry(Config *config);

#endif
