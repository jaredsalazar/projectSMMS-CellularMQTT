#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "Config.h"
#include "utilities.h"
#include "Telemetry.h"

bool sdReady = false;
char sdLogFileName[40];

void buildSdLogFileName()
{
    // CSV is simple for the TTGO to append and opens directly in Excel.
    snprintf(sdLogFileName, sizeof(sdLogFileName), "/sense-%s.csv", deviceId);
}

bool beginSdLogger()
{
    buildSdLogFileName();

    SPI.begin(BOARD_SD_SCK_PIN, BOARD_SD_MISO_PIN, BOARD_SD_MOSI_PIN, BOARD_SD_CS_PIN);
    sdReady = SD.begin(BOARD_SD_CS_PIN, SPI);
    if (!sdReady) {
        SerialMon.println("SD card init failed; measurements will only be sent over MQTT.");
        return false;
    }

    const bool needsHeader = !SD.exists(sdLogFileName);
    File logFile = SD.open(sdLogFileName, FILE_APPEND);
    if (!logFile) {
        sdReady = false;
        SerialMon.print("Could not open SD log file ");
        SerialMon.println(sdLogFileName);
        return false;
    }

    if (needsHeader) {
        logFile.println("device_id,firmware_version,uptime_ms,battery_mv,battery_percent,solar_mv,solar_charging,rssi,gps_valid,gps_lat,gps_lon,gps_alt,ads_ok,analog0_raw,analog0_voltage,analog1_raw,analog1_voltage,analog2_raw,analog2_voltage,analog3_raw,analog3_voltage");
    }

    logFile.close();
    SerialMon.print("SD logging to ");
    SerialMon.println(sdLogFileName);
    return true;
}

bool appendMeasurementToSd(const SensorReadings &sensors, const GpsFix &gps, int rssi)
{
    if (!sdReady) {
        return false;
    }

    File logFile = SD.open(sdLogFileName, FILE_APPEND);
    if (!logFile) {
        SerialMon.print("Could not append to SD log file ");
        SerialMon.println(sdLogFileName);
        return false;
    }

    logFile.printf("%s,%s,%lu,%lu,%u,%lu,%s,%d,%s,%.6f,%.6f,%.1f,%s,%d,%.3f,%d,%.3f,%d,%.3f,%d,%.3f\n",
                   deviceId,
                   FIRMWARE_VERSION,
                   static_cast<unsigned long>(millis()),
                   static_cast<unsigned long>(sensors.batteryMilliVolts),
                   sensors.batteryPercent,
                   static_cast<unsigned long>(sensors.solarMilliVolts),
                   boolText(sensors.solarCharging),
                   rssi,
                   boolText(gps.valid),
                   gps.valid ? gps.latitude : 0.0f,
                   gps.valid ? gps.longitude : 0.0f,
                   gps.valid ? gps.altitude : 0.0f,
                   boolText(sensors.adsOk),
                   sensors.analog[0].raw, sensors.analog[0].voltage,
                   sensors.analog[1].raw, sensors.analog[1].voltage,
                   sensors.analog[2].raw, sensors.analog[2].voltage,
                   sensors.analog[3].raw, sensors.analog[3].voltage);
    logFile.close();

    SerialMon.print("Measurement saved to SD ");
    SerialMon.println(sdLogFileName);
    return true;
}
