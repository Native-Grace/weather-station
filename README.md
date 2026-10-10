# Weather Station (SparkFun MicroMod)

ESP32 MicroMod firmware for the [SparkFun MicroMod Weather Carrier Board](https://www.sparkfun.com/products/16794).

Reads local sensors, serves UDP clients, and uploads observations to Weather Underground (PWS).

| Item | Value |
|------|--------|
| Firmware version | `1.1.0` |
| Target | SparkFun ESP32 MicroMod (`esp32micromod`) |
| Config | SD card `/config.jsn` (see `example/config.jsn`) |
| Hostname | `Weather Station 1` |

## Hardware

- BME280 (I2C): humidity, pressure, internal temperature
- Dallas 1-Wire on `G0`: outdoor temperature (`tempf` for WU)
- Weather Meter Kit: wind speed/direction, rain (pins 35 / 14 / 27)
- AS3935 lightning (SPI CS=`25`) — optional; station continues if absent
- SD card: required for config

## Weather Underground upload

Uploads use the classic **PWS RapidFire / updateraw** HTTP(S) API.

### When it runs

In `loop()`, if Wi-Fi/NTP is synced and `weather_underground.use` is true, `updateWU()` runs every `upd8_frequency` seconds.

### Request shape

1. `create_url_str()` builds the query string:

```text
?action=updateraw
&ID=<device_id>
&PASSWORD=<device_key>
&softwaretype=WeatherStation_ESP32
&dateutc=YYYY-MM-DD+HH%3AMM%3ASS
&humidity=...
&baromin=...
&tempf=...
&windspeedmph=...
&winddir=...
&windgustmph=...
&dailyrainin=...
&rainin=...
```

2. `updateWU()` sends:

```http
GET <endpoint><query>&realtime=1&rtfreq=<upd8_frequency> HTTP/1.1
Host: <host>
Connection: close
```

Default host/endpoint (see example config):

- Host: `rtupdate.wunderground.com`
- Endpoint: `/weatherstation/updateweatherstation.php`
- Protocol: `https` (port 443) or `http` (port 80)

### Field mapping

| WU parameter | Source |
|--------------|--------|
| `ID` / `PASSWORD` | Station ID and **station key** from config (`device_id` / `device_key`) |
| `dateutc` | NTP UTC time, URL-encoded |
| `humidity`, `baromin` | BME280 (pressure converted mb → inHg) |
| `tempf` | Dallas outdoor sensor (°F) |
| `windspeedmph`, `winddir` | Weather meter (kph → mph) |
| `windgustmph` | Peak wind over last ~10 minutes (`GUSTINTERVAL`) |
| `dailyrainin`, `rainin` | Daily / last-hour rain (mm → inches) |

Notes:

- BME280 outdoor `tempf` is **not** sent; outdoor temp comes from the Dallas probe.
- Serial logs redact `PASSWORD=…`.
- Optional UDP clients with `reportWU` receive a copy of the request line after a successful connect.

### Config keys

```json
"weather_underground": {
  "use": true,
  "protocol": "https",
  "host": "rtupdate.wunderground.com",
  "endpoint": "/weatherstation/updateweatherstation.php",
  "device_id": "ISTATION123",
  "device_key": "xxxxxxxx",
  "upd8_frequency": 60
}
```

Put the real file on the SD card as `/config.jsn`. Do not commit secrets.

## UDP control

Messages start with `AAAAAAAA` plus a type character. See comments in `config.h` for listen/add/config message formats. Default listen port: `49999`.

## Build / flash

Arduino IDE or PlatformIO with ESP32 MicroMod board support. Required libraries include:

- ArduinoJson
- SparkFun BME280
- SparkFun AS3935
- SparkFun Weather Meter Kit
- NTP (sstaub)
- OneWire / DallasTemperature
- TimeLib

Flash over the CP2102N USB-UART (`/dev/cu.usbserial-*` on macOS).

## Changelog (1.1.0)

- No longer hard-hangs on missing USB serial, SD, config, or Wi-Fi (retries instead)
- AS3935 failure no longer marks the detector as OK
- Fixed Australia AEST/AEDT NTP rule assignment
- Fixed WU HTTP request (path-only GET; removed duplicate `action=updateraw`)
- Redacted WU station key from serial logs
