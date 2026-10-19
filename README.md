# Weather Station (SparkFun MicroMod)

ESP32 MicroMod firmware for the [SparkFun MicroMod Weather Carrier Board](https://www.sparkfun.com/products/16794).

Reads local sensors, serves UDP clients, and uploads observations to Weather Underground (PWS).

| Item | Value |
|------|--------|
| Firmware version | `1.4.0` |
| Target | SparkFun ESP32 MicroMod (`esp32micromod`) |
| Config | SD card `/config.jsn` (see `example/config.jsn`) |
| Hostname | `Weather Station 1` |

## Hardware

- BME280 (I2C): humidity, pressure (used for outdoor humidity / barometer; indoor temps not uploaded)
- Dallas 1-Wire on `G0`: up to 2× DS18B20 (`tempf` / `temp2f` for WU; ROM-sorted)
- VEML6075 (I2C): UV index
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
&dewptf=...
&windspeedmph=...
&winddir=...
&windgustmph=...
&windgustdir=...
&windgustmph_10m=...
&windgustdir_10m=...
&windspdmph_avg2m=...
&winddir_avg2m=...
&dailyrainin=...
&rainin=...
&UV=...
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
| `tempf` | Dallas sensor 0 (°F, lowest ROM address) |
| `temp2f` | Dallas sensor 1 (°F, if present) |
| `dewptf` | Computed from Dallas `tempf` + BME humidity |
| `windspeedmph`, `winddir` | Instantaneous weather meter (kph → mph) |
| `windgustmph`, `windgustdir` | Peak over last ~10 minutes (`GUSTINTERVAL`) |
| `windgustmph_10m`, `windgustdir_10m` | Same 10‑minute gust window |
| `windspdmph_avg2m`, `winddir_avg2m` | ~2‑minute rolling average (vector mean for direction) |
| `dailyrainin`, `rainin` | Daily / last-hour rain (mm → inches) |
| `UV` | VEML6075 UV index (when sensor responds) |

Notes:

- Indoor fields (`indoortempf`, `indoorhumidity`) are **not** uploaded.
- Soil moisture is not enabled: carrier soil power conflicts with Dallas on `G0`.
- `solarradiation` / `leafwetness` need sensors this board does not have.
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

## Changelog

### 1.4.0

- MQTT + Home Assistant discovery (both Dallas temps, humidity, wind, rain, board temp)
- See `docs/homeassistant.md`

### 1.3.2

- Weather Underground `temp2f` comes from Dallas sensor 1 again (not BME280)

### 1.3.1

- Briefly used onboard BME280 as `temp2f` when only one Dallas was present

### 1.3.0

- Support two DS18B20 sensors on the G0 1-Wire bus
- Stable sensor index by sorted ROM address

### 1.2.0

- Upload additional WU fields: dewpoint, gust direction, 10‑minute gusts, 2‑minute wind averages, UV index
- Enable VEML6075 when present
- Still omit indoor temperature/humidity

### 1.1.0

- No longer hard-hangs on missing USB serial, SD, config, or Wi-Fi (retries instead)
- AS3935 failure no longer marks the detector as OK
- Fixed Australia AEST/AEDT NTP rule assignment
- Fixed WU HTTP request (path-only GET; removed duplicate `action=updateraw`)
- Redacted WU station key from serial logs
