/*loop_epoch
 * MicroMod Weather Carrier Board Example
 * 
 * This sketch tests all of the weather sensors on the carrier board:
 * atmospheric sensor - BME280, UV sensor - VEML6075, lightning detector - AS3935,
 * soil moisture sensor, wind and rain meters.
 * 
 * Priyanka Makin @ SparkX Labs
 * Original Creation Date: August 20, 2020
 * 
 * Hardware Connections:
 * Insert MicroMod processor board of your choice into the M.2 connector of the SparkFun Weather carrier
 *  Screw into place
 * Connect Weather carrier board to power using USB-C cable
 * Connect SparkFun Soil Moisture Sensor to Weather carrier using latching terminals
 * Connect both wind and rain meters to Weather carrier using the RJ11 connectors
 * 
 * Updated be Wes Furuya
 * 07/10/2023
 * Implemented "Weather Meter" Arduino library
 * Updated pins for other processor boards
 */

#define ESP32 true
#include "defines.h"
#include "weathertypes.h"
#include "Arduino.h"
#include "config.h"
#include "networks_funcs.h"
#include "Readings.h"
#include <SPI.h>
#include "SparkFunBME280.h"
#include "SparkFun_AS3935.h"
#include "SparkFun_VEML6075_Arduino_Library.h"
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"  //http://librarymanager/All#SparkFun_Weather_Meter_Kit
#include <WiFiClientSecure.h>
#include <TimeLib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


const char* CACert = "-----BEGIN CERTIFICATE-----\n" \
"MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n" \
"iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n" \
"cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n" \
"BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n" \
"MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n" \
"BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n" \
"aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n" \
"dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n" \
"AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n" \
"3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n" \
"tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n" \
"Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n" \
"VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n" \
"79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n" \
"c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n" \
"Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n" \
"c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n" \
"UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n" \
"Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n" \
"BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n" \
"A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n" \
"Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n" \
"VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n" \
"ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n" \
"8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n" \
"iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n" \
"Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n" \
"XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n" \
"qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n" \
"VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n" \
"L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n" \
"jjxDah2nGN59PRbxYvnKkKj9\n" \
"-----END CERTIFICATE-----\n";



WiFiUDP Udp;
NTP ntp(Udp);

JsonDocument udpdoc;


bool synced_time = false;
char packetBuffer[4096]; 
unsigned long secondsBetweenWU = 0;

const char* RESPONSES[] = { "success\n", "failed\n", "denied\n", "too many clients\n", "here\n" , 
                          "Listeners Added\nActive Clients Added\n", "Listeners Added\nActive Clients Update Failed\n",
                          "Listener Removed\nActive Clients Removed\n"
                          };
UDP_CLIENTS_TYPE udp_clients[MAX_CLIENTS];
unsigned long lastWUupd8;
unsigned long lastUDPupd8;
bool lightningDetected = false;

int rainperday_day;

// int LED_BUILTIN = 5;
// variables used as temporary storages
// these are used in various functions to reduce the use of the stack
char *longlengthTemp;
char *midlengthTemp;
time_t loop_epoch = ntp.epoch();
time_t setup_epoch = ntp.epoch();
int loop_day = ntp.day();
char smalltemp0[128];
char smalltemp1[128];

#define GUSTINTERVAL 600
long gusttimer;
#define RAININTERVAL  60
// ~2 minutes of samples at loop delay(~2s)
#define WIND_AVG_SAMPLES 60
float lasthourrain = 0;
float raintoday = 0.0;
time_t raintimer = 0;
float rainIn10 = 0;
float maxwind = 0.0;
int maxwinddir = 0;
float currentwind = 0.0;
float maxwind4wu = 0.0;
int maxwinddir4wu = 0;
float wind_speed_samples[WIND_AVG_SAMPLES];
float wind_dir_samples[WIND_AVG_SAMPLES];
int wind_sample_idx = 0;
int wind_sample_count = 0;

float rainarray[RAINARRAY_LENGTH];
float *rainarrayhead = &rainarray[0];
float *rainarrayend = &rainarray[RAINARRAY_LENGTH - 1];

BME280 tempSensor;
VEML6075 uv;
SparkFun_AS3935 lightning;

#define RRSIZE 4096
char *working_char = (char *)malloc(512);
char *loop_outstring = (char *)malloc(1024);
char *readRequest = (char *)malloc(RRSIZE);
char smalltemp[128];

int windDirectionPin = 35;
int windSpeedPin = 14;
int rainfallPin = 27;

// Soil moisture uses A0 + a power GPIO on the carrier. G0 is used for the
// Dallas outdoor temp probe, so soil moisture is not enabled in this build.
int WHITE_WIRE = G0;  // Dallas OneWire bus (supports multiple DS18B20s)
OneWire oneWire(WHITE_WIRE);
DallasTemperature dallas_temperature(&oneWire);
DeviceAddress dallasAddresses[MAX_DALLAS_SENSORS];
uint8_t dallasCount = 0;

// const int lightningInt = 17; // Interrupt pin for lightning detection
int spiCS = 25; //G1SPI chip select pin

bool VEML_ok = false;
bool BME_ok = false;
bool AS3935_ok = false;


// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 
int intVal = 0;
int noise = 2; // Value between 1-7 
int disturber = 2; // Value between 1-10
SFEWeatherMeterKit weatherMeterKit(windDirectionPin, windSpeedPin, rainfallPin);

char webPayload[2000];
WiFiClient httpclient;
WiFiClientSecure httpsclient;

Config config;

const char* filename = "/config.jsn";  // <- SD library uses 8.3 filenames


void readHttpResponse(WiFiClient *client) {
 
  // uint32_t received_data_num = 0;
  while (client->available()) {
   char c = client->read();
    Serial.print(c);
  }  
}

void calibrateWindDirection() {
    SFEWeatherMeterKitCalibrationParams calibrationParams = weatherMeterKit.getCalibrationParams();
    calibrationParams.vaneADCValues[WMK_ANGLE_0_0] = 2903;
    calibrationParams.vaneADCValues[WMK_ANGLE_22_5] = 1408;
    calibrationParams.vaneADCValues[WMK_ANGLE_45_0] = 1662;
    calibrationParams.vaneADCValues[WMK_ANGLE_67_5] = 164;
    calibrationParams.vaneADCValues[WMK_ANGLE_90_0] = 196;
    calibrationParams.vaneADCValues[WMK_ANGLE_112_5] = 98;
    calibrationParams.vaneADCValues[WMK_ANGLE_135_0] = 548;
    calibrationParams.vaneADCValues[WMK_ANGLE_157_5] = 319;
    calibrationParams.vaneADCValues[WMK_ANGLE_180_0] = 949;
    calibrationParams.vaneADCValues[WMK_ANGLE_202_5] = 944;
    calibrationParams.vaneADCValues[WMK_ANGLE_225_0] = 2283;
    calibrationParams.vaneADCValues[WMK_ANGLE_247_5] = 2192;
    calibrationParams.vaneADCValues[WMK_ANGLE_270_0] = 3863;
    calibrationParams.vaneADCValues[WMK_ANGLE_292_5] = 3453;
    calibrationParams.vaneADCValues[WMK_ANGLE_315_0] = 3456;
    calibrationParams.vaneADCValues[WMK_ANGLE_337_5] = 2582;

    calibrationParams.mmPerRainfallCount = 0.3636; /* <-- set this to calibrated value */

    weatherMeterKit.setCalibrationParams(calibrationParams);
}

void clearUserInput()
{
    // Ensure all previous characters have come through
    delay(100);

    // Throw away all previous characters
    while (Serial.available() != 0)
    {
        Serial.read();
    }
}

void waitForUserInput()
{
    // Remove previous user input
    clearUserInput();

    // Wait for user to input something
    while (Serial.available() == 0)
    {
        // Nothing to do, keep waiting
    }
}

static void appendQueryFloat(char *param_str, const char *key, float value, int decimals) {
  strcat(param_str, key);
  dtostrf(value, 1, decimals, smalltemp0);
  strcat(param_str, smalltemp0);
}

static void appendQueryInt(char *param_str, const char *key, int value) {
  sprintf(working_char, "%s%d", key, value);
  strcat(param_str, working_char);
}

// Magnus-Tetens dewpoint (°C) from temperature (°C) and relative humidity (%).
static float dewpointC(float tempC, float humidityPct) {
  if (humidityPct <= 0.0f) {
    humidityPct = 0.01f;
  }
  if (humidityPct > 100.0f) {
    humidityPct = 100.0f;
  }
  const float a = 17.27f;
  const float b = 237.7f;
  float gamma = (a * tempC) / (b + tempC) + logf(humidityPct / 100.0f);
  return (b * gamma) / (a - gamma);
}

static float celsiusToFahrenheit(float tempC) {
  return ((9.0f * tempC) / 5.0f) + 32.0f;
}

void recordWindSample(float speedKph, float dirDeg) {
  wind_speed_samples[wind_sample_idx] = speedKph;
  wind_dir_samples[wind_sample_idx] = dirDeg;
  wind_sample_idx = (wind_sample_idx + 1) % WIND_AVG_SAMPLES;
  if (wind_sample_count < WIND_AVG_SAMPLES) {
    wind_sample_count++;
  }
}

float avgWindSpeedKph() {
  if (wind_sample_count == 0) {
    return 0.0f;
  }
  float sum = 0.0f;
  for (int i = 0; i < wind_sample_count; i++) {
    sum += wind_speed_samples[i];
  }
  return sum / (float)wind_sample_count;
}

float avgWindDirDeg() {
  if (wind_sample_count == 0) {
    return 0.0f;
  }
  // Vector mean so 359° / 1° averages correctly near north.
  double x = 0.0;
  double y = 0.0;
  for (int i = 0; i < wind_sample_count; i++) {
    double r = radians(wind_dir_samples[i]);
    x += cos(r);
    y += sin(r);
  }
  float deg = (float)degrees(atan2(y, x));
  if (deg < 0.0f) {
    deg += 360.0f;
  }
  return deg;
}

static void printDallasAddress(const DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] < 16) {
      Serial.print('0');
    }
    Serial.print(addr[i], HEX);
    if (i < 7) {
      Serial.print(':');
    }
  }
}

static int compareDallasAddress(const void *a, const void *b) {
  return memcmp(a, b, 8);
}

static bool dallasTempValidC(float tempC) {
  return tempC > -55.0f && tempC < 85.0f;
}

static void initDallasSensors() {
  dallas_temperature.begin();
  uint8_t found = dallas_temperature.getDeviceCount();
  dallasCount = 0;

  Serial.print(F("Found "));
  Serial.print(found, DEC);
  Serial.println(F(" Dallas devices."));

  for (uint8_t i = 0; i < found && dallasCount < MAX_DALLAS_SENSORS; i++) {
    if (dallas_temperature.getAddress(dallasAddresses[dallasCount], i)) {
      dallasCount++;
    }
  }

  // Stable index assignment across reboots (ROM address order).
  if (dallasCount > 1) {
    qsort(dallasAddresses, dallasCount, sizeof(DeviceAddress), compareDallasAddress);
  }

  for (uint8_t i = 0; i < dallasCount; i++) {
    dallas_temperature.setResolution(dallasAddresses[i], 12);
    Serial.print(F("  Dallas["));
    Serial.print(i);
    Serial.print(F("] "));
    printDallasAddress(dallasAddresses[i]);
    if (i == 0) {
      Serial.print(F(" -> tempf"));
    } else if (i == 1) {
      Serial.print(F(" -> temp2f"));
    }
    Serial.println();
  }

  Serial.print(F("Parasite power is: "));
  if (dallas_temperature.isParasitePowerMode()) {
    Serial.println(F("ON"));
  } else {
    Serial.println(F("OFF"));
  }
}

char *create_url_str(char* param_str) {
  float outdoorTempC = 0.0f;
  float outdoorTempF = 0.0f;
  float humidityPct = 0.0f;
  bool haveOutdoorTemp = false;
  bool haveHumidity = false;

  param_str[0] = 0;
  // Weather Underground PWS RapidFire / updateraw query string.
  // ID = station ID, PASSWORD = station key (not the website login password).
  // Indoor fields (indoortempf / indoorhumidity) intentionally omitted.
  // Dallas[0] -> tempf, Dallas[1] -> temp2f (ROM-sorted).
  sprintf(param_str,
          "?action=updateraw&ID=%s&PASSWORD=%s&softwaretype=WeatherStation_ESP32&dateutc=",
          config.weather_underground.device_id,
          config.weather_underground.device_key);
  ntp.update();
  time_t utc = ntp.epoch();
  struct tm *t1 = gmtime(&utc);
  strftime(working_char, 128, "%Y-%m-%d+%H%%3A%M%%3A%S", t1);
  strcat(param_str, working_char);

  if (BME_ok) {
    humidityPct = tempSensor.readFloatHumidity();
    haveHumidity = true;
    appendQueryFloat(param_str, "&humidity=", humidityPct, 0);
    appendQueryFloat(param_str, "&baromin=", mb2in(tempSensor.readFloatPressure()), 3);
  }

  if (dallasCount > 0) {
    dallas_temperature.requestTemperatures();
  }

  for (uint8_t i = 0; i < dallasCount; i++) {
    float tempC = dallas_temperature.getTempC(dallasAddresses[i]);
    float tempF = dallas_temperature.getTempF(dallasAddresses[i]);
    if (!dallasTempValidC(tempC)) {
      continue;
    }
    if (i == 0) {
      outdoorTempC = tempC;
      outdoorTempF = tempF;
      haveOutdoorTemp = true;
      appendQueryFloat(param_str, "&tempf=", tempF, 1);
    } else if (i == 1) {
      appendQueryFloat(param_str, "&temp2f=", tempF, 1);
    }
    Serial.print(F("Dallas["));
    Serial.print(i);
    Serial.print(F("]: "));
    Serial.print(tempC, 2);
    Serial.print(F(" C / "));
    Serial.print(tempF, 2);
    Serial.println(F(" F"));
  }

  if (BME_ok) {
    Serial.print(F("BME280: "));
    Serial.print(tempSensor.readTempC(), 2);
    Serial.print(F(" C / "));
    Serial.print(tempSensor.readTempF(), 2);
    Serial.println(F(" F"));
  }

  if (haveOutdoorTemp && haveHumidity) {
    appendQueryFloat(param_str, "&dewptf=",
                     celsiusToFahrenheit(dewpointC(outdoorTempC, humidityPct)), 1);
  }

  float windSpeedKph = weatherMeterKit.getWindSpeed();
  float windDirDeg = weatherMeterKit.getWindDirection();
  appendQueryFloat(param_str, "&windspeedmph=", windSpeedKph / 1.609344f, 2);
  appendQueryInt(param_str, "&winddir=", (int)lroundf(windDirDeg));

  appendQueryFloat(param_str, "&windgustmph=", maxwind4wu / 1.609344f, 2);
  appendQueryInt(param_str, "&windgustdir=", maxwinddir4wu);
  // GUSTINTERVAL is 600s, so current gust window is the 10-minute window.
  appendQueryFloat(param_str, "&windgustmph_10m=", maxwind4wu / 1.609344f, 2);
  appendQueryInt(param_str, "&windgustdir_10m=", maxwinddir4wu);

  if (wind_sample_count > 0) {
    appendQueryFloat(param_str, "&windspdmph_avg2m=", avgWindSpeedKph() / 1.609344f, 2);
    appendQueryInt(param_str, "&winddir_avg2m=", (int)lroundf(avgWindDirDeg()));
  }

  strcat(param_str, dailyRain2in(smalltemp0));
  strcat(param_str, hourlyRain2in(smalltemp0));

  if (VEML_ok) {
    appendQueryFloat(param_str, "&UV=", uv.index(), 2);
  }

  return param_str;
}

char *createUDPData(char* datastr, UNITS2REPORT *units) {

  datastr[0] = 0;
  strcpy(datastr, VALID_MESSAGE_HEADER);
  strcat(datastr,"1{");
  sprintf(working_char, "\"time\": \"%s\",", ntp.formattedTime("%Y-%m-%d %H:%M:%S"));
  strcat(datastr, working_char);
  if (BME_ok){
    if ( units->humidity) {
      sprintf(working_char,"\"humidity\": %.1f%s", tempSensor.readFloatHumidity(), units->commaAfter_humidity() ? "," : "");
      strcat(datastr, working_char);
    }
    if(units->temperatureI){
      sprintf(working_char,"\"internal_temperature\":%.1f%s", tempSensor.readTempC(),units->commaAfter_temperatureI() ? "," : "");
      strcat(datastr, working_char);
    }
    if(units->pressure){
      sprintf(working_char,"\"pressure\": %.1f%s", tempSensor.readFloatPressure()/100, units->commaAfter_pressure() ? "," : "");
      strcat(datastr, working_char);
    }
  } else {
    strcat(datastr, "\"humidity\": \"error\", \"internal temperature\": \"error\",\"pressure\": \"error\"");
    if(units->commaAfter_pressure()) {
      strcat(datastr, ",");
    }
  }
  if(units->temperatureX){
    if (dallasCount > 0) {
      dallas_temperature.requestTemperatures();
    }
    if (dallasCount > 0 && dallasTempValidC(dallas_temperature.getTempC(dallasAddresses[0]))) {
      dtostrf(dallas_temperature.getTempC(dallasAddresses[0]), 1, 1, smalltemp1);
      sprintf(working_char, "\"external_temperature\":%s", smalltemp1);
      strcat(datastr, working_char);
      if (dallasCount > 1 && dallasTempValidC(dallas_temperature.getTempC(dallasAddresses[1]))) {
        dtostrf(dallas_temperature.getTempC(dallasAddresses[1]), 1, 1, smalltemp1);
        sprintf(working_char, ",\"external_temperature2\":%s", smalltemp1);
        strcat(datastr, working_char);
      }
      if (units->commaAfter_temperatureX()) {
        strcat(datastr, ",");
      }
    }
  }
  if(units->speed){
    dtostrf(weatherMeterKit.getWindSpeed(),1,2,smalltemp1);
    sprintf(working_char,"\"windspeed\": %s,", smalltemp1 );
    strcat(datastr, working_char);

    dtostrf(maxwind4wu,1,2,smalltemp1);
    sprintf(working_char,"\"windgustkph\": %s%s", smalltemp1, units->commaAfter_speed() ? "," : "");
    strcat(datastr, working_char);

  }
  if(units->direction){
    sprintf(working_char,"\"windir\": %.1f%s", weatherMeterKit.getWindDirection(), units->commaAfter_direction() ? "," : "");
    strcat(datastr, working_char);
  }
  if(units->rain){
    dtostrf(raintoday,1,1,smalltemp1);
    sprintf(working_char,"\"rain2day\": %s,", smalltemp1);
    strcat(datastr, working_char);

    dtostrf(lasthourrain,1,1,smalltemp1);
    sprintf(working_char,"\"rainLasthr\": %s%s", smalltemp1, units->commaAfter_rain() ? "," : "");
    strcat(datastr, working_char);
  }
  if(units->lightning){
    sprintf(working_char,"\"lightning\": %s", (lightningDetected) ? "true" : "false");
    strcat(datastr, working_char);
    lightningDetected = false;
  }  
  strcat(datastr,"}\n");
  return datastr;
}

char *createUDPData_tabbed(char* datastr, UNITS2REPORT *units) {
  
  int fields_printed = 0;
  datastr[0] = 0;
  sprintf(working_char, "Time: %s\n", ntp.formattedTime("%Y-%m-%d %H:%M:%S"));
  strcat(datastr, working_char);
  if (BME_ok){
    if ( units->humidity) {
      sprintf(working_char,"Humidity: %.1f\t\t\t", tempSensor.readFloatHumidity());
      strcat(datastr, working_char);
      fields_printed++;
    }
    if(units->temperatureI){
      sprintf(working_char,"Internal Temperature: %.1f", tempSensor.readTempC());
      strcat(datastr, working_char);
      if(fields_printed >= 2) {
        fields_printed = 0;
        strcat(datastr,"\n");
      } else {
        strcat(datastr,"\t");
        fields_printed++;
      }
    }
    if(units->pressure){
      sprintf(working_char,"Pressure: %.1f", tempSensor.readFloatPressure()/100);
      strcat(datastr, working_char);
      if(fields_printed >= 2) {
        fields_printed = 0;
        strcat(datastr,"\n");
      } else {
        strcat(datastr,"\t\t");
        fields_printed++;
      }
    }
  } else {
    if ( units->humidity) {
      strcat(datastr, "Humidity: Error\t\t");
      fields_printed++;
    }
    if(units->temperatureI){
      strcat(datastr, "Temperature: Error");
      if(fields_printed >= 2) {
        fields_printed = 0;
        strcat(datastr,"\n");
      } else {
        strcat(datastr,"\t\t");
        fields_printed++;
      }
    }
    if(units->pressure){
      strcat(datastr, "Pressure: Error");
      if(fields_printed >= 2) {
        fields_printed = 0;
        strcat(datastr,"\n");
      } else {
        strcat(datastr,"\t\t");
        fields_printed++;
      }
    }
  }
  if(units->temperatureX){
    if (dallasCount > 0) {
      dallas_temperature.requestTemperatures();
    }
    if (dallasCount > 0 && dallasTempValidC(dallas_temperature.getTempC(dallasAddresses[0]))) {
      dtostrf(dallas_temperature.getTempC(dallasAddresses[0]), 1, 1, smalltemp1);
      sprintf(working_char, "External Temperature: %s", smalltemp1);
      Serial.println(working_char);
      strcat(datastr, working_char);
      if (fields_printed >= 2) {
        fields_printed = 0;
        strcat(datastr, "\n");
      } else {
        strcat(datastr, "\t");
        fields_printed++;
      }
    }
    if (dallasCount > 1 && dallasTempValidC(dallas_temperature.getTempC(dallasAddresses[1]))) {
      dtostrf(dallas_temperature.getTempC(dallasAddresses[1]), 1, 1, smalltemp1);
      sprintf(working_char, "External Temperature 2: %s", smalltemp1);
      Serial.println(working_char);
      strcat(datastr, working_char);
      if (fields_printed >= 2) {
        fields_printed = 0;
        strcat(datastr, "\n");
      } else {
        strcat(datastr, "\t");
        fields_printed++;
      }
    }
  }

  if(units->speed){
    dtostrf(weatherMeterKit.getWindSpeed(),1,2,smalltemp1);
    sprintf(working_char,"Windspeed (kph): %s", smalltemp1 );
    strcat(datastr, working_char);
    if(fields_printed >= 2) {
      fields_printed = 0;
      strcat(datastr,"\n");
    } else {
      strcat(datastr,"\t\t");
      fields_printed++;
    }

    dtostrf(maxwind4wu,1,2,smalltemp1);
    sprintf(working_char,"Wind Gust (kph): %s", smalltemp1);
    strcat(datastr, working_char);
    if(fields_printed >= 2) {
      fields_printed = 0;
      strcat(datastr,"\n");
    } else {
      strcat(datastr,"\t\t");
      fields_printed++;
    }

  }
  if(units->direction){
    sprintf(working_char,"Wind Direction: %.1f", weatherMeterKit.getWindDirection());
    strcat(datastr, working_char);
    if(fields_printed >= 2) {
      fields_printed = 0;
      strcat(datastr,"\n");
    } else {
      strcat(datastr,"\t\t");
      fields_printed++;
    }
  }

  if(units->rain){
    dtostrf(raintoday,1,1,smalltemp1);
    sprintf(working_char,"Rain Today (mm): %s,", smalltemp1);
    strcat(datastr, working_char);
    if(fields_printed >= 2) {
      fields_printed = 0;
      strcat(datastr,"\n");
    } else {
      strcat(datastr,"\t\t");
      fields_printed++;
    }

    dtostrf(lasthourrain,1,1,smalltemp1);
    sprintf(working_char,"Rain in Last Hour: %s", smalltemp1);
    strcat(datastr, working_char);
    if(fields_printed >= 2) {
      fields_printed = 0;
      strcat(datastr,"\n");
    } else {
      strcat(datastr,"\t\t");
      fields_printed++;
    }
  }
  if(units->lightning){
    sprintf(working_char,"Lightning: %s", (lightningDetected) ? "true" : "false");
    strcat(datastr, working_char);
    if(fields_printed >= 2) {
      fields_printed = 0;
      strcat(datastr,"\n");
    } else {
      strcat(datastr,"\t\t");
      fields_printed++;
    }
    lightningDetected = false;
  }  
  strcat(datastr,"\n\n");
  return datastr;
}

static bool initSdCard() {
#ifdef SPARKFUN_THING_PLUS_C
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, sd_cs);
  if (!SD.begin(sd_cs)) {
    Serial.println(F("setup: Sparkfun Thing Plus C Card Mount Failed"));
    return false;
  }
  Serial.println(F("setup: Sparkfun Thing Plus C Card Mount Succeeded"));
#else
  SPI.begin();
  if (!SD.begin()) {
    Serial.println(F("setup: SD Begin Failed"));
    return false;
  }
  Serial.println(F("setup: Card Begin Succeeded"));
#endif
  return true;
}

static bool initSensorsAndNetwork() {
  int8_t udp_started;

  if (!loadConfiguration(filename, &config, true)) {
    Serial.println(F("setup: Config NOT Loaded"));
    return false;
  }

  Serial.println(F("setup: Config Loaded OK"));
  if (config.weather_underground.upd8_frequency) {
    secondsBetweenWU = config.weather_underground.upd8_frequency;
    Serial.print(F("Weather Underground updates every: "));
    Serial.print(config.weather_underground.upd8_frequency);
    Serial.println(F(" Seconds"));
  }

  if (!start_wifi(&config, &ntp, false)) {
    Serial.println(F("setup: WIFI NOT Connected"));
    return false;
  }

  udp_started = Udp.begin(config.udp.port);
  if (udp_started) {
    ntp.updateInterval(5000);
    synced_time = true;
    UNITS2REPORT units;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (config.udp.listeners[i].port >= MIN_UDP_PORT &&
          config.udp.listeners[i].port <= MAX_UDP_PORT) {
        addNewListener(udp_clients, config.udp.listeners[i].ip,
                       config.udp.listeners[i].port, &units, UDP_SOURCE_CONFIG);
      }
    }
  }
  Serial.print(F("setup: UDP Port: "));
  Serial.println(config.udp.port);
  Serial.print(F("setup: UDP status: "));
  Serial.println(udp_started ? "Started" : "Failed");

  if (tempSensor.beginI2C() == false) {
    Serial.println(F("BME280 did not respond."));
    BME_ok = false;
  } else {
    BME_ok = true;
  }

  pinMode(LED_BUILTIN, OUTPUT);

  if (lightning.beginSPI(spiCS, 2000000) == false) {
    Serial.println(F("Lightning Detector did not start up; continuing without it."));
    AS3935_ok = false;
  } else {
    Serial.println(F("Schmow-ZoW, Lightning Detector Ready!"));
    lightning.setIndoorOutdoor(OUTDOOR);
    AS3935_ok = true;
  }

  rainperday_day = ntp.day();
  setup_epoch = ntp.epoch();
  raintimer = setup_epoch + RAININTERVAL;
  gusttimer = setup_epoch + GUSTINTERVAL;
  lastWUupd8 = (unsigned long)ntp.epoch();
  lastUDPupd8 = lastWUupd8;

  httpsclient.setCACert(CACert);
  Serial.println(F("Setup Complete"));
  return true;
}

void setup() {
  Serial.begin(115200);
  // Wait briefly for USB serial; do not hang forever on headless boots.
  unsigned long serial_wait_start = millis();
  while (!Serial && (millis() - serial_wait_start) < 3000) {
    delay(10);
  }
  Serial.print(F("Weather Station firmware "));
  Serial.println(FIRMWARE_VERSION);

  for (int i = 0; i < RAINARRAY_LENGTH; i++) {
    rainarray[i] = 0.0;
  }

  httpsclient.setCACert(CACert);
  PRINTLN("About to check if calibrate wind direction");
  calibrateWindDirection();
  PRINTLN("About to check start weatherMetreKit");
  weatherMeterKit.begin();

  while (!initSdCard()) {
    Serial.println(F("setup: Retrying SD card in 5s..."));
    delay(5000);
  }

  Wire.begin();
  initDallasSensors();

  if (uv.begin() == false) {
    Serial.println(F("VEML6075 did not respond."));
    VEML_ok = false;
  } else {
    Serial.println(F("VEML6075 UV sensor ready."));
    VEML_ok = true;
  }

  while (!initSensorsAndNetwork()) {
    Serial.println(F("setup: Retrying config/Wi-Fi in 15s..."));
    delay(15000);
  }
}

// the loop function runs over and over again forever
void loop() {
  PRINTLN("Geting time and day");
  loop_epoch = ntp.epoch();
  loop_day = ntp.day();
  currentwind = weatherMeterKit.getWindSpeed();
  float currentdir = weatherMeterKit.getWindDirection();
  recordWindSample(currentwind, currentdir);
  if (currentwind > maxwind) {
    maxwind = currentwind;
    maxwinddir = (int)lroundf(currentdir);
  }
  if(rainperday_day != loop_day) {
    rainperday_day = loop_day;
    raintoday = 0.0;
  } 

  if (loop_epoch >= gusttimer) {
      ntp.update();
      maxwind4wu = maxwind;
      maxwinddir4wu = maxwinddir;
      maxwind = currentwind;
      gusttimer = ntp.epoch() + GUSTINTERVAL;
  }
  if (loop_epoch >= raintimer) {
    rainIn10 = weatherMeterKit.getTotalRainfall();
    Serial.print("Rain this interval: ");
    Serial.println(rainIn10);
    weatherMeterKit.resetTotalRainfall();
    *rainarrayhead = rainIn10;
    if (rainarrayhead < rainarrayend) {
      rainarrayhead++;
    } else {
      rainarrayhead = &rainarray[0];
    }
    raintoday += rainIn10;
    raintimer = loop_epoch + RAININTERVAL;
    Serial.print("Rain today: ");
    Serial.println(raintoday);
    storeLastHourRain();
  }


  handleUDPRequests();
  // Serial.println("Checking if ready to send UDP");
  if (config.udp.use && config.udp.update_frequency){
    //Serial.println("Ready to send UDP");
    if ((unsigned long)now() - lastUDPupd8 >= config.udp.update_frequency) {
      lastUDPupd8 = (unsigned long)now();
      //Serial.println("About to send UDP");
      sendUDPMessages(udp_clients,loop_outstring , &Udp );
    }
  }
  // Serial.println("Checking if ready to send WU");
  if (synced_time){
    if (config.weather_underground.use && secondsBetweenWU){
      // Serial.println("Ready to send WU update");
      if ((unsigned long)ntp.epoch() - lastWUupd8 >= secondsBetweenWU) {
        // Serial.println("About to send WU Update");
        updateWU();
      }
    }
  }
  // digitalWrite(LED_BUILTIN, LED_ON ? LOW : HIGH);
  // LED_ON = !LED_ON;

  delay(2000);
}

static void logWuRequestSanitized(const char *request_line) {
  // Avoid printing the station key/password in serial logs.
  const char *pwd = strstr(request_line, "PASSWORD=");
  Serial.print(ntp.formattedTime("%Y-%m-%d %H:%M:%S"));
  Serial.print(F(" Sending WU request: "));
  if (!pwd) {
    Serial.println(request_line);
    return;
  }
  const char *pwd_val = pwd + strlen("PASSWORD=");
  Serial.write(request_line, (size_t)(pwd_val - request_line));
  Serial.print(F("****"));
  const char *after = strchr(pwd_val, '&');
  if (after) {
    Serial.println(after);
  } else {
    Serial.println();
  }
}

void updateWU() {
    lastWUupd8 = (unsigned long)ntp.epoch();
    // Query string already includes action=updateraw from create_url_str().
    // Use a path-only request-target (HTTP/1.1), plus realtime/rtfreq for rapid updates.
    sprintf(readRequest, "GET %s%s&realtime=1&rtfreq=%d HTTP/1.1",
            config.weather_underground.endpoint,
            create_url_str(webPayload),
            config.weather_underground.upd8_frequency);
    logWuRequestSanitized(readRequest);

    bool connected = false;
    if (strcmp(config.weather_underground.protocol, "http") == 0) {
      connected = httpclient.connect(config.weather_underground.host,
                                     config.weather_underground.port);
      if (connected) {
        httpclient.println(readRequest);
        httpclient.print(F("Host: "));
        httpclient.println(config.weather_underground.host);
        httpclient.println(F("Connection: close"));
        httpclient.println();
        readHttpResponse((WiFiClient *)&httpclient);
        httpclient.stop();
      }
    } else {
      connected = httpsclient.connect(config.weather_underground.host,
                                      config.weather_underground.port);
      if (connected) {
        httpsclient.println(readRequest);
        httpsclient.print(F("Host: "));
        httpsclient.println(config.weather_underground.host);
        httpsclient.println(F("Connection: close"));
        httpsclient.println();
        readHttpResponse((WiFiClient *)&httpsclient);
        httpsclient.stop();
      }
    }

    if (!connected) {
      readRequest[0] = 0;
      Serial.print(F("Unable to connect to "));
      Serial.println(config.weather_underground.host);
    } else {
      strcat(readRequest, "\n");
      reportWUtoUDP(udp_clients, readRequest, &Udp);
    }
}

void storeLastHourRain() {
    float total = 0.0;
    for( float *p = &rainarray[0]; p <= rainarrayend; p++) {
      total += *p;
    }
    lasthourrain = total;
    Serial.print("Rain Last Hour: ");
    Serial.println(total);
}

char *hourlyRain2in(char* result) {
  dtostrf(lasthourrain/25.4,1,1,smalltemp1);
  sprintf(result, "&rainin=%s", smalltemp1);
  return result;
}

char *dailyRain2in(char* result) {
  dtostrf(raintoday/25.4,1,1,smalltemp1);
  sprintf(result, "&dailyrainin=%s", smalltemp1);
  return result;
}

// int readSoil() {
//   int moistVal = 0;  //Variable for storing moisture value
//   //Power Senor
//   digitalWrite(soilPower, HIGH);
//   delay(10);
//   moistVal = analogRead(soilPin);  //Read the SIG value from sensor
//   digitalWrite(soilPower, LOW); //Turn the sensor off
//   return moistVal; //Return current moisture value
// }

char *kmh2mph(char* result, float kph) {
  dtostrf(kph/1.609344,1,2,smalltemp1);
  sprintf(result, "%s", smalltemp1);
  return result;
}

char *ms2mph(char* result, float ms) {
  sprintf(result, "%.2f", ms*2.23693629);
  return result;
}

char *c2f(char* result, float c) {
  sprintf(result, "%.2f", ((9*c)/5)+32);
  return result;
}

char *mm2ft(char* result, float mm) {
  sprintf(result, "%.2f", mm/25.4);
  return result;
}

float mb2in(float mb) {
  return mb/3386.38864;
}


void removeListener(UDP_CLIENTS_TYPE * clients, IPAddress ip, port_t port) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if( clients[i].use && clients[i].source == UDP_SOURCE_REQUEST && clients[i].ip == ip &&  clients[i].port == port ) {
      clients[i].use = false;
      Serial.print(F("removeListener: Removed UDP Listener IP: "));
      Serial.print(ip.toString());
      Serial.print(F(" port: "));
      Serial.println(port);
    }
  }
}

int findlistener(UDP_CLIENTS_TYPE * clients, IPAddress ip, port_t port) {
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].ip == ip && clients[i].port == port) {
      return i;
    }
  }
  return -1;
}

char *listlisteners(UDP_CLIENTS_TYPE * clients, char *list_str) {
  bool listener_found=false;
  char port[8];
  list_str[0] = 0;
  PRINTLN("listlisteners: Started looking");
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].use) {
      listener_found = true;
      sprintf(list_str, "Client Index: %d,", i);
      strcat(list_str, "IPAddress: ");
      strcat(list_str, clients[i].ip.toString().c_str());
      strcat(list_str,",");
      dtostrf(clients[i].port,1,0,port);
      strcat(list_str, "Port: ");
      strcat(list_str,port);
      strcat(list_str,",");
      strcat(list_str, "Source: ");
      strcat(list_str, (clients[i].source == UDP_SOURCE_CONFIG) ? "Config File," : "Network Request,");
      strcat(list_str, "Send Weather Underground Parameters: ");
      strcat(list_str, (clients[i].reportWU) ? "True," : "False,");
      strcat(list_str, "Send Weather Underground Parameters Only: ");
      strcat(list_str, (clients[i].reportWUonly) ? "True," : "False,");
      strcat(list_str, "Send Humity: ");
      strcat(list_str, (clients[i].units2report.humidity) ? "True," : "False,");
      strcat(list_str, "Send Internal Temperature: ");
      strcat(list_str, (clients[i].units2report.temperatureI) ? "True," : "False,");
      strcat(list_str, "Send External Temperature: ");
      strcat(list_str, (clients[i].units2report.temperatureX) ? "True," : "False,");
      strcat(list_str, "Send Pressure: ");
      strcat(list_str, (clients[i].units2report.pressure) ? "True," : "False,");
      strcat(list_str, "Send Wind Speed: ");
      strcat(list_str, (clients[i].units2report.speed) ? "True," : "False,");
      strcat(list_str, "Send Wind Direction: ");
      strcat(list_str, (clients[i].units2report.direction) ? "True," : "False,");
      strcat(list_str, "Send Rainfall: ");
      strcat(list_str, (clients[i].units2report.rain) ? "True," : "False,");
      strcat(list_str, "Send Lightning: ");
      strcat(list_str, (clients[i].units2report.lightning) ? "True\n" : "False\n");
      PRINTLN(list_str);
    } else {
      dtostrf(i,1,0,port);
      PRINT("Client ");
      PRINT(port);
      PRINTLN(" slot is free");

    }
  }
  if(!listener_found) {
    strcpy(list_str,"There Are No Listeners\n");
  }
  return list_str;
}

bool addNewListener(UDP_CLIENTS_TYPE * clients, IPAddress ip, port_t port, UNITS2REPORT *units) {
  return addNewListener(clients, ip, port, units, UDP_SOURCE_REQUEST);
}

bool addNewListener(UDP_CLIENTS_TYPE *clients, IPAddress ip, port_t port, UNITS2REPORT *units, char source) {
  int index = findlistener(clients, ip, port);
  if (index >= 0) {
      clients[index].use = true;
      clients[index].source = source;
      memcpy(&clients[index].units2report, units, sizeof(clients[index].units2report));
      clients[index].reportWU = units->reportWU;
      clients[index].reportWUonly = units->reportWUonly;

      Serial.print(F("addNewListener: Updated UDP Listener IP: "));
      Serial.print(ip.toString());
      Serial.print(F(" port: "));
      Serial.println(port);
      return true;
  } else {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if( !clients[i].use && port >= MIN_UDP_PORT && port <= MAX_UDP_PORT) {
        clients[i].ip = ip;
        clients[i].port = port;
        clients[i].use = true;
        clients[i].source = source;
        if(units->pretty) {
          clients[i].pretty = true;
        }
        if(units->reportWUonly) {
          clients[i].reportWU = true;
        } else {
          clients[i].reportWU = units->reportWU;
        }
        clients[i].reportWUonly = units->reportWUonly;
        memcpy(&clients[i].units2report, units, sizeof(clients[i].units2report));

        Serial.print(F("addNewListener: Added UDP Listener IP: "));
        Serial.print(ip.toString());
        Serial.print(F(" port: "));
        Serial.println(port);

        return true;
      }
    }
  }
  return false;
}

void sendUDPMessages(UDP_CLIENTS_TYPE * clients, char *msg, WiFiUDP *udp) {
    for( int i = 0; i < MAX_CLIENTS; i++){
      if(clients[i].use && !clients[i].reportWUonly){
        if(clients[i].pretty){
          createUDPData_tabbed(msg, &clients[i].units2report);
        } else {
          createUDPData(msg, &clients[i].units2report);
        }
        sendUDPMessage(clients[i].ip, clients[i].port, msg, udp);
      }
    }
}

void reportWUtoUDP(UDP_CLIENTS_TYPE * clients, char *msg, WiFiUDP *udp) {
    for( int i = 0; i < MAX_CLIENTS; i++){
      if(clients[i].use && clients[i].reportWU && msg[0]){
        sendUDPMessage(clients[i].ip, clients[i].port, msg, udp);
      }
    }
}

void reloadConfig(const char* filename, Config* config, bool reloadConfig_flag) {
  if(loadConfiguration(filename, config, true)) {
    // clone current client list
    UDP_CLIENTS_TYPE current_udp_clients[MAX_CLIENTS];
    memcpy((void *)&current_udp_clients, (void *)udp_clients, sizeof(udp_clients));

    // Stop current Udp connection
    Udp.stop();
    delay(300);

    // restart Udp
    int8_t udp_started = Udp.begin(config->udp.port);

    Serial.println(F("reloadConfig: After ReConfig:"));
    Serial.print(F("              UDP Port: "));
    Serial.println(config->udp.port);
    Serial.print(F("              UDP status: "));
    Serial.println(udp_started ? "Running" : "Failed to start");
    

    if (udp_started) {

      // clear current client list
      for (int i = 0; i < MAX_CLIENTS; i++) {
        udp_clients[i].use = false;
      }

      //add clients for default listeners
      UNITS2REPORT units;
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if(config->udp.listeners[i].port >= MIN_UDP_PORT && config->udp.listeners[i].port <= MAX_UDP_PORT) {
          addNewListener(udp_clients, config->udp.listeners[i].ip, config->udp.listeners[i].port, &units, UDP_SOURCE_CONFIG );
        }
      }

      // add as many of current clients as possible back
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if(current_udp_clients[i].use && current_udp_clients[i].source == UDP_SOURCE_REQUEST){
          addNewListener(udp_clients, current_udp_clients[i].ip, current_udp_clients[i].port, &current_udp_clients[i].units2report );
        }
      }
    }
  }
}

void handleUDPRequests() {
  int type_index;
  char type;
  // Serial.println("About to check if udp message has come in");
  int packetSize = Udp.parsePacket();
  PRINT("Size of udp message :");
  PRINTLN(packetSize);
  if (packetSize) {
    int len = Udp.read(packetBuffer, 4096);
    PRINT(F("loop: Packet Received Length: "));
    PRINTLN(len);

    if (len > 0)
    {
      packetBuffer[len] = 0;
    }
    Serial.print(F("loop: Packet Received: "));
    Serial.println(packetBuffer);
    type_index = isValidMessage(packetBuffer);
    if (type_index) {
      type = packetBuffer[type_index];
      switch(type) {
        case '3': // add new listener
          {
            bool oktoadd = true;
            UNITS2REPORT units;
            Serial.println("Received request");
            Serial.print("packetBuffer[type_index+1]: ");
            Serial.println((byte)packetBuffer[type_index+1]);
            if((char)packetBuffer[type_index+1] == '{'){
              PRINTLN("about to deserialize");
              DeserializationError err = deserializeJson(udpdoc, &packetBuffer[type_index+1]);
              if(err) {
                Serial.println ("deserialize error");
                oktoadd = false;
              } else {
                getRequestedData(&units, udpdoc);
              }
            }
            if(oktoadd){
              if (addNewListener(udp_clients, Udp.remoteIP(), Udp.remotePort(), &units )){
                sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_SUCCESS], &Udp );
              } else {
                sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_2MANY], &Udp );
              }
            }
          }
          break;
        case '2': // return current values
          {
            char outstr[1024];
            UNITS2REPORT units;
            sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),createUDPData(outstr, &units), &Udp );
          } 
          break;
        case '4':  // handshake
          sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_HERE], &Udp );
          break;
        case '5':  // removes active listener
          if((char)packetBuffer[type_index+1] != '{'){
            removeListener(udp_clients, Udp.remoteIP(), Udp.remotePort() );
            sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_SUCCESS], &Udp );
          } else {
            DeserializationError err = deserializeJson(udpdoc, &packetBuffer[type_index+1]);
            if (!err) {
              if (udpdoc["listenerindex"].is<signed int>()) {
                int index = udpdoc["listenerindex"].as<signed int>();
                if (index >= 0 && index < MAX_CLIENTS){
                  udp_clients[index].use = false;
                  sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_SUCCESS], &Udp );
                }  else {
                  sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_FAILED], &Udp );
                }
              } else {
                sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_FAILED], &Udp );
              }
            } else {
              sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_FAILED], &Udp );
            }
          }
          break;
        case '6': // edit various configuration options
          {
            Serial.println("handleUDPRequests: change configuration requested");
            DeserializationError err = deserializeJson(udpdoc, &packetBuffer[type_index+1]);
            Serial.println(&packetBuffer[type_index+1]);
            if (!err) {
              bool restartWiFi;
              if(editConfiguration(filename, udpdoc, &restartWiFi)) {
                reloadConfig(filename, &config, restartWiFi);
                sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_SUCCESS], &Udp );
              } else {
                sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_FAILED], &Udp );
              }
            } else {
              Serial.print("handleUDPRequests: Bad change configuration request");
            }
          }          
          break;
        case 'C':  // returns parts of configuration file
          {
            DeserializationError err = deserializeJson(udpdoc, &packetBuffer[type_index+1]);
            if (!err) {
              char buffer[2048];
              bool success;
              getConfigPart(filename, udpdoc, buffer, sizeof(buffer), &success );
              if(success) {
                sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),buffer, &Udp );
              } else {
                sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),(char *)RESPONSES[RESPONSES_FAILED], &Udp );
              }
            } 
          }
          break;
        case 'U':
          Serial.println("Received UDP list Listeners");
          listlisteners(udp_clients, loop_outstring);
          PRINT("loop_outstring : ");
          PRINT(loop_outstring);
          sendUDPMessage(Udp.remoteIP(), Udp.remotePort(),loop_outstring, &Udp );
          break;
      }

    }
  }

}

int isValidMessage(char *msg) {
  int comparelen = strlen(VALID_MESSAGE_HEADER);
  int i;
  for (i = 0; i < comparelen; i++ ) {
    if(msg[i] != VALID_MESSAGE_HEADER[i]) {
      return 0;
    }
  }
  return i;
}


