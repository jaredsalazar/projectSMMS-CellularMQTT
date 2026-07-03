# projectSMMS-CellularMQTT Handover

## Current State

This project is an Arduino firmware sketch for a Soil Moisture Monitoring System using a LilyGo T-A7670 / A7670SA ESP32 board, Cellular LTE, MQTT, GPS, battery/solar monitoring, and an ADS1115 over I2C.

The active main sketch is:

- `projectSMMS-CellularMQTT.ino`

Additional split sketches now exist:

- `CellularOTA/CellularOTA.ino`: OTA-capable 5-minute sleep sketch.
- `projectSMMS-CellularMQTT-NoOTA/projectSMMS-CellularMQTT-NoOTA.ino`: preserved non-OTA 5-minute sleep sketch.

It currently runs in low-power cycle mode:

1. Boot ESP32.
2. Enable the LilyGo battery power rail.
3. Initialize ADS1115, modem, LTE/GPRS, MQTT, and GPS.
4. Publish one telemetry JSON message to HiveMQ.
5. Disconnect MQTT/GPRS.
6. Power off the modem.
7. Enter ESP32 deep sleep for 5 minutes.
8. Wake and repeat.

The previous always-on version was copied to:

- `15sec_StaysON/projectSMMS-CellularMQTT-15sec_StaysON.ino`

That backup version publishes every 15 seconds and keeps the modem on.

## Important Settings

Configured in `Config.h`:

- MQTT broker: `broker.hivemq.com`
- MQTT port: `1883`
- MQTT topic: `sapat/test`
- APN: `internet`
- Active sleep interval: `5 minutes`
- Backup always-on publish interval: `15 seconds`

The board profile is in `utilities.h` and targets `LILYGO_T_A7670`.

Important LilyGo pins:

- Modem TX: GPIO 26
- Modem RX: GPIO 27
- Modem PWRKEY: GPIO 4
- Modem RESET: GPIO 5
- Battery power rail enable: GPIO 12
- Battery ADC: GPIO 35
- Solar ADC: GPIO 36
- I2C SDA: GPIO 21
- I2C SCL: GPIO 22

## File Map

- `projectSMMS-CellularMQTT.ino`: active 5-minute sleep-mode sketch.
- `CellularOTA/`: self-contained Arduino sketch copy with HTTP-over-cellular OTA support.
- `projectSMMS-CellularMQTT-NoOTA/`: self-contained Arduino sketch copy without OTA support.
- `15sec_StaysON/projectSMMS-CellularMQTT-15sec_StaysON.ino`: backup always-on 15-second sketch.
- `Config.h`: MQTT, APN, intervals, ADC constants, payload size.
- `utilities.h`: LilyGo T-A7670 board pin definitions.
- `BoardPower.h`: board pin setup, battery rail hold, modem power-on.
- `Sensors.h`: ADS1115 scan/read, battery voltage, solar voltage, battery percentage.
- `ModemMqtt.h`: TinyGSM modem setup, LTE/GPRS reconnect, MQTT reconnect, RSSI, MQTT publish.
- `Gps.h`: GPS enable/read using TinyGSM A7670 APIs.
- `Telemetry.h`: device ID and JSON telemetry payload builder.

## Telemetry Payload

The JSON payload includes:

- `device_id`
- `uptime_ms`
- `battery_mv`
- `battery_percent`
- `solar_mv`
- `solar_charging`
- `rssi`
- `gps.valid`
- `gps.lat`
- `gps.lon`
- `gps.alt`
- `ads_ok`
- `analog[0..3].raw`
- `analog[0..3].voltage`

## Battery / Sleep Notes

Battery operation depends on `BOARD_POWERON_PIN` / GPIO 12 being set HIGH early at boot. This is implemented in `BoardPower.h`.

The active sketch calls:

- `holdBatteryPowerRail()`
- `esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_US)`
- `esp_deep_sleep_start()`

Before sleep, it also calls:

- `mqtt.disconnect()`
- `modem.gprsDisconnect()`
- `modem.poweroff()`

If the modem should remain powered during ESP32 sleep, remove or change `modem.poweroff()` in `sleepUntilNextSend()` inside `projectSMMS-CellularMQTT.ino`.

## Known Caveats

The `CellularOTA` sketch uses first-pass plain HTTP OTA. Configure `OTA_HOST`,
`OTA_VERSION_PATH`, and `OTA_BINARY_PATH` in `CellularOTA/Config.h` before using
it in the field, and select an ESP32 partition scheme with OTA app slots in
Arduino IDE.

This environment did not have `arduino-cli`, PlatformIO, or `git` on PATH, so the sketch has not been compiled from this shell.

The code was checked against the local reference project:

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\projectSapat-ClientMQTT`

And LilyGo reference repo:

- `C:\Users\jared salazar\OneDrive\Documents\GitHub\LilyGo-Modem-Series`

The TinyGSM fork from LilyGo is required. The sketch intentionally keeps this guard:

```cpp
#ifndef TINY_GSM_FORK_LIBRARY
#error "Please install the LilyGo TinyGSM fork from LilyGo-Modem-Series/lib."
#endif
```

## Suggested Next Steps

1. Open `projectSMMS-CellularMQTT.ino` in Arduino IDE.
2. Select ESP32 Dev Module and the LilyGo-compatible ESP32 settings.
3. Confirm LilyGo TinyGSM fork and PubSubClient are installed in Arduino libraries.
4. Compile and upload.
5. Watch serial logs for:
   - battery rail enabled
   - ADS1115 detected
   - modem initialized
   - network connected
   - MQTT connected
   - telemetry published
   - entering 5 minute deep sleep
6. Subscribe to `sapat/test` on `broker.hivemq.com:1883` to confirm payload delivery.
