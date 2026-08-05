# projectSMMS-CellularMQTT Handover

## Current State

This project is an Arduino firmware sketch for a Soil Moisture Monitoring System using a LilyGo T-A7670 / A7670SA ESP32 board, Cellular LTE, MQTT, GPS, battery/solar monitoring, an ADS1115 over I2C, and SD card CSV logging.

The active main sketch is:

- `projectSMMS-CellularMQTT.ino`

Additional split sketches now exist:

- `CellularOTA/CellularOTA.ino`: OTA-capable 5-minute sleep sketch.
- `projectSMMS-CellularMQTT-NoOTA/projectSMMS-CellularMQTT-NoOTA.ino`: preserved non-OTA 5-minute sleep sketch.

It currently runs in low-power cycle mode:

1. Boot ESP32.
2. Enable the LilyGo battery power rail.
3. Initialize SD logging and briefly power the ADS1115 rail for ADC detection.
4. Initialize modem, LTE/GPRS, MQTT, and GPS.
5. Read one measurement.
6. Append the measurement to SD card CSV first.
7. Publish the same measurement as one telemetry JSON message to HiveMQ if the modem is ready.
8. Disconnect MQTT/GPRS.
9. Power off the modem.
10. Hold sensor power disabled and enter ESP32 deep sleep for 5 minutes.
11. Wake and repeat.

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
- Firmware version: `FIRMWARE_VERSION`, format `X.YZ`

The board profile is in `utilities.h` and targets `LILYGO_T_A7670`.

Important LilyGo pins:

- Modem TX: GPIO 26
- Modem RX: GPIO 27
- Modem PWRKEY: GPIO 4
- Modem RESET: GPIO 5
- Battery power rail enable: GPIO 12
- Switched sensor power enable: GPIO 19
- Battery ADC: GPIO 35
- Solar ADC: GPIO 36
- I2C SDA: GPIO 21
- I2C SCL: GPIO 22
- SD SCK: GPIO 14
- SD MISO: GPIO 2
- SD MOSI: GPIO 15
- SD CS: GPIO 13

## File Map

- `projectSMMS-CellularMQTT.ino`: active 5-minute sleep-mode sketch.
- `CellularOTA/`: self-contained Arduino sketch copy with HTTP-over-cellular OTA support.
- `projectSMMS-CellularMQTT-NoOTA/`: self-contained Arduino sketch copy without OTA support.
- `15sec_StaysON/projectSMMS-CellularMQTT-15sec_StaysON.ino`: backup always-on 15-second sketch.
- `SdCardCsvTest/SdCardCsvTest.ino`: standalone TTGO A7670SA SD-card CSV write test. It appends `this is an sd card test` to `/sd_test.csv` every 10 seconds and prints serial diagnostics.
- `Ads1115I2cPowerTest/Ads1115I2cPowerTest.ino`: standalone ADS1115/I2C diagnostic. It enables GPIO 12, toggles the sensor power trigger on GPIO 19 HIGH/LOW, and scans the configured I2C pins for any device.
- `Config.h`: MQTT, APN, intervals, ADC constants, payload size.
- `utilities.h`: LilyGo T-A7670 board pin definitions.
- `BoardPower.h`: board pin setup, battery rail hold, modem power-on.
- `Sensors.h`: ADS1115 scan/read using the Adafruit ADS1X15 library, battery voltage, solar voltage, battery percentage.
- `ModemMqtt.h`: TinyGSM modem setup, LTE/GPRS reconnect, MQTT reconnect, RSSI, MQTT publish.
- `Gps.h`: GPS enable/read using TinyGSM A7670 APIs.
- `Telemetry.h`: device ID and JSON telemetry payload builder.
- `DataLogger.h`: SD card initialization and Excel-readable CSV append logging.

## Telemetry Payload

The JSON payload includes:

- `device_id`
- `firmware_version`
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

## SD Card Logging

The active sketch writes each measurement to the SD card before attempting MQTT publish.

The TTGO A7670SA built-in microSD slot uses LilyGo's board-specific SPI pins, not the ESP32 default VSPI pins. GPIO 12 must be driven HIGH to keep the board power rail enabled. GPIO 19 is held HIGH during SD initialization and SD append in the active sketch in case an external SD module or sensor rail is wired to the switched sensor supply.

The log file name is:

- `/sense-(device id).csv`

Example:

- `/sense-smms-1234ABCD.csv`

The file is created if it does not exist. If it is newly created, the first row is a CSV header. Each later wake cycle appends one new measurement row. CSV was chosen because it is easy for the TTGO/ESP32 to write and can be opened directly in Excel for data analysis.

The CSV includes:

- `device_id`
- `firmware_version`
- `uptime_ms`
- battery, solar, RSSI, GPS, ADS1115 status, and analog channel values

If the SD card cannot initialize, the sketch prints an error and continues with MQTT-only operation. If the modem fails, the sketch still logs the measurement to SD with `rssi = -999` and does not publish.

Default SD SPI pins are defined in `utilities.h`:

- `BOARD_SD_SCK_PIN`: GPIO 14
- `BOARD_SD_MISO_PIN`: GPIO 2
- `BOARD_SD_MOSI_PIN`: GPIO 15
- `BOARD_SD_CS_PIN`: GPIO 13

These pins match the TTGO/LilyGo A7670SA built-in microSD slot. Only use GPIO 18/23 if you are wiring a separate external SD module to those pins and update `utilities.h` accordingly. GPIO 19 is now used as the switched ADS1115/sensor power trigger.

## Firmware Versioning

The active sketch defines firmware version in `Config.h`:

```cpp
const char FIRMWARE_VERSION[] = "1.03";
```

Version format is `X.YZ`:

- `X`: major update
- `Y`: minor update
- `Z`: minor correction

Update `FIRMWARE_VERSION` whenever firmware behavior changes. The version is printed at startup, included in MQTT telemetry as `firmware_version`, and written to every SD CSV row.

## Battery / Sleep Notes

Battery operation depends on `BOARD_POWERON_PIN` / GPIO 12 being set HIGH early at boot. This is implemented in `BoardPower.h`.

External sensor power is controlled by `BOARD_SENSOR_POWER_EN_PIN` / GPIO 19. It is active-high:

- GPIO 19 HIGH: ADS1115/sensor power enabled.
- GPIO 19 LOW: ADS1115/sensor power disabled.
- The sketch pulls GPIO 19 HIGH only while initializing or reading the ADS1115.
- Before deep sleep, `sleepUntilNextSend()` calls `setSensorPowerEnabled(false)` and `holdSensorPowerOffDuringSleep()` so GPIO 19 is held LOW through ESP32 deep sleep.

The active sketch calls:

- `holdBatteryPowerRail()`
- `setSensorPowerEnabled(true)` only when the ADS1115 needs power
- `esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_US)`
- `esp_deep_sleep_start()`

Before sleep, it also calls:

- `mqtt.disconnect()`
- `modem.gprsDisconnect()`
- `modem.poweroff()`
- `setSensorPowerEnabled(false)`
- `holdSensorPowerOffDuringSleep()`

If the modem should remain powered during ESP32 sleep, remove or change `modem.poweroff()` in `sleepUntilNextSend()` inside `projectSMMS-CellularMQTT.ino`.

## ADS1115 / I2C Debug Notes

The TTGO A7670SA SD card and ADS1115 I2C bus do not share pins:

- I2C SDA: GPIO 21
- I2C SCL: GPIO 22
- SD SCK: GPIO 14
- SD MISO: GPIO 2
- SD MOSI: GPIO 15
- SD CS: GPIO 13

The active firmware expects the ADS1115 at `0x48` because the ADS1115 `ADDR` pin is tied to GND.

Debug result from `Ads1115I2cPowerTest` on the connected TTGO A7670SA (`COM8`, `115200` baud):

- GPIO 12 board power rail was driven HIGH.
- GPIO 19 was tested HIGH and LOW.
- I2C scan found no devices on SDA/SCL `21/22`.
- The expanded scan also found no devices on `22/21`, `17/18`, `18/17`, or `4/5`.

This means the current ADS1115 failure is not caused by the corrected SD SPI pins. Most likely causes are hardware-side: ADS1115 VCC/GND not powered, no common ground, SDA/SCL not wired to GPIO 21/22, missing pullups on the I2C lines, incorrect connector/pin header, or a faulty/absent ADS1115 module.

## Known Caveats

If GPIO 19 still measures high during deep sleep after firmware upload, check the hardware. The firmware intends to hold this active-high enable pin LOW while sleeping. Possible causes are wiring mistakes, a missing pullup/pulldown appropriate to the load switch, or backfeed through I2C pullups/sensor protection diodes. Make sure switched sensors are not being powered indirectly through SDA/SCL.

The `CellularOTA` sketch uses first-pass plain HTTP OTA. Configure `OTA_HOST`,
`OTA_VERSION_PATH`, and `OTA_BINARY_PATH` in `CellularOTA/Config.h` before using
it in the field, and select an ESP32 partition scheme with OTA app slots in
Arduino IDE.

This environment has a local `arduino-cli` installed at `.tools/arduino-cli.exe`. The active sketch and the standalone SD-card CSV test compile with `esp32:esp32:esp32`.

The SD-card CSV test was uploaded to the connected TTGO A7670SA on `COM8` and verified over serial at `115200` baud. The board mounted an SDHC/SDXC card and appended rows to `/sd_test.csv` every 10 seconds.

The board was later flashed with `Ads1115I2cPowerTest` for I2C diagnostics. Re-upload `projectSMMS-CellularMQTT.ino` or `SdCardCsvTest/SdCardCsvTest.ino` after ADS1115 hardware debugging.

The code was checked against the local reference project:

- `F:\GitHub\projectSapat-ClientMQTT`

And LilyGo reference repo:

- `F:\GitHub\LilyGo-Modem-Series`

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
   - SD logging file opened
   - ADS1115 detected
   - modem initialized
   - network connected
   - MQTT connected
   - telemetry published
   - entering 5 minute deep sleep
6. Subscribe to `sapat/test` on `broker.hivemq.com:1883` to confirm payload delivery.
