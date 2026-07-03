# CellularOTA Arduino Sketch

Open `CellularOTA.ino` in Arduino IDE to build the OTA-capable sketch.

Before uploading, choose an ESP32 partition scheme with OTA support. Do not use a
`No OTA` partition scheme.

Update these placeholders in `Config.h` before field testing:

- `OTA_HOST`
- `OTA_VERSION_PATH`
- `OTA_BINARY_PATH`
- `FIRMWARE_VERSION`

The first-pass OTA flow uses plain HTTP over the existing TinyGSM cellular data
connection. Host `version.txt` and the exported compiled `.bin` file at the
configured paths.
