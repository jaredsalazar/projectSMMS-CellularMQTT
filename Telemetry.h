#pragma once

#include <Arduino.h>
#include "Config.h"
#include "Sensors.h"
#include "Gps.h"

char deviceId[24];
char mqttClientId[32];
char telemetryPayload[TELEMETRY_PAYLOAD_SIZE];

void buildDeviceIdentity()
{
    const uint64_t mac = ESP.getEfuseMac();
    const uint32_t chipId = static_cast<uint32_t>(mac);
    snprintf(deviceId, sizeof(deviceId), "smms-%08lX", static_cast<unsigned long>(chipId));
    snprintf(mqttClientId, sizeof(mqttClientId), "smms-mqtt-%08lX", static_cast<unsigned long>(chipId));
}

const char *boolText(bool value)
{
    return value ? "true" : "false";
}

const char *buildTelemetryPayload(const SensorReadings &sensors, const GpsFix &gps, int rssi)
{
    snprintf(telemetryPayload, sizeof(telemetryPayload),
             "{\"device_id\":\"%s\",\"uptime_ms\":%lu,"
             "\"battery_mv\":%lu,\"battery_percent\":%u,"
             "\"solar_mv\":%lu,\"solar_charging\":%s,"
             "\"rssi\":%d,"
             "\"gps\":{\"valid\":%s,\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f},"
             "\"ads_ok\":%s,"
             "\"analog\":["
             "{\"channel\":0,\"raw\":%d,\"voltage\":%.3f},"
             "{\"channel\":1,\"raw\":%d,\"voltage\":%.3f},"
             "{\"channel\":2,\"raw\":%d,\"voltage\":%.3f},"
             "{\"channel\":3,\"raw\":%d,\"voltage\":%.3f}"
             "]}",
             deviceId,
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

    return telemetryPayload;
}
