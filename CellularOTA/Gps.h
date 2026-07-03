#pragma once

#include <Arduino.h>
#include "Config.h"
#include "utilities.h"
#include "ModemMqtt.h"

struct GpsFix {
    bool valid;
    float latitude;
    float longitude;
    float altitude;
    float accuracy;
    uint8_t status;
    uint32_t updatedAtMs;
};

bool gpsEnabled = false;
GpsFix lastGpsFix = {};

void initializeGps()
{
#ifdef TINY_GSM_MODEM_HAS_GPS
    if (MODEM_GPS_ENABLE_GPIO >= 0 && MODEM_GPS_ENABLE_LEVEL >= 0) {
        gpsEnabled = modem.enableGPS(MODEM_GPS_ENABLE_GPIO, MODEM_GPS_ENABLE_LEVEL);
    } else {
        gpsEnabled = modem.enableGPS();
    }

    if (gpsEnabled) {
        modem.enableAGPS();
        SerialMon.println("GPS enabled");
    } else {
        SerialMon.println("GPS enable failed");
    }
#else
    gpsEnabled = false;
    SerialMon.println("GPS API not available for this modem build");
#endif
}

GpsFix readGpsFix()
{
    if (!gpsEnabled) {
        return lastGpsFix;
    }

    if (lastGpsFix.updatedAtMs != 0 && millis() - lastGpsFix.updatedAtMs < GPS_REFRESH_INTERVAL_MS) {
        return lastGpsFix;
    }

    uint8_t status = 0;
    float latitude = 0.0f;
    float longitude = 0.0f;
    float speed = 0.0f;
    float altitude = 0.0f;
    int visibleSatellites = 0;
    int usedSatellites = 0;
    float accuracy = 0.0f;

    const bool ok = modem.getGPS(&status, &latitude, &longitude, &speed, &altitude,
                                 &visibleSatellites, &usedSatellites, &accuracy);

    lastGpsFix.updatedAtMs = millis();
    if (ok && status > 0 && usedSatellites > 0) {
        lastGpsFix.valid = true;
        lastGpsFix.latitude = latitude;
        lastGpsFix.longitude = longitude;
        lastGpsFix.altitude = altitude;
        lastGpsFix.accuracy = accuracy;
        lastGpsFix.status = status;
    } else {
        lastGpsFix.valid = false;
    }

    return lastGpsFix;
}
