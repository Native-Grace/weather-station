# Home Assistant (Synology Docker)

The weather station publishes MQTT with Home Assistant discovery. Both Dallas temps show as sensors.

## 1. Mosquitto on Synology

In Container Manager, run Mosquitto (or include it in the same compose as Home Assistant).

Example `mosquitto.conf`:

```conf
listener 1883
allow_anonymous true
persistence true
persistence_location /mosquitto/data/
```

Broker address = your Synology LAN IP, port `1883`.

## 2. Home Assistant MQTT integration

1. Open HA → **Settings → Devices & services → Add integration → MQTT**
2. Broker: Synology IP  
3. Port: `1883`  
4. Leave user/password blank if using `allow_anonymous true`

## 3. Station SD card `/config.jsn`

Add (use your Synology IP):

```json
"mqtt": {
  "use": true,
  "host": "192.168.2.10",
  "port": 1883,
  "user": "",
  "password": "",
  "base_topic": "weatherstation",
  "discovery_prefix": "homeassistant",
  "upd8_frequency": 30
}
```

Reboot the MicroMod after saving the SD config.

## 4. What appears in HA

Device **Weather Station** with sensors including:

- Outdoor Temperature (`temp_c` / Dallas[0])
- Temperature 2 (`temp2_c` / Dallas[1])
- Board Temperature (BME280)
- Humidity, Pressure, Wind, Rain

State topic: `weatherstation/state`

## 5. Serial check

You should see:

```text
MQTT: connected to 192.168.2.10
MQTT: Home Assistant discovery published
MQTT: published weatherstation/state
```
