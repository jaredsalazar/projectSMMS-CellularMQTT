#include "../Config.h"
#include "../utilities.h"
#include "../BoardPower.h"
#include "../Sensors.h"
#include "../ModemMqtt.h"
#include "../Gps.h"
#include "../Telemetry.h"

uint32_t lastPublish = 0;
bool modemReady = false;

void setup()
{
    SerialMon.begin(SERIAL_BAUD);
    delay(500);

    SerialMon.println();
    SerialMon.println("SMMS Cellular MQTT starting - 15sec StaysON");
    SerialMon.println(PRODUCT_MODEL_NAME);

    buildDeviceIdentity();
    SerialMon.print("Device ID: ");
    SerialMon.println(deviceId);

    configureBoardPins();
    holdBatteryPowerRail();
    setSensorPowerEnabled(true);
    delay(50);
    configureAnalogInputs();
    beginAds1115();
    setSensorPowerEnabled(false);

    powerOnModem();
    modemReady = initializeModem();
    if (modemReady) {
        ensureNetworkConnection(true);
        ensureGprsConnection();
        initializeGps();
    }
}

void loop()
{
    if (!modemReady) {
        delay(1000);
        return;
    }

    maintainConnections(mqttClientId);

    const uint32_t now = millis();
    if (mqtt.connected() && (lastPublish == 0 || now - lastPublish >= PUBLISH_INTERVAL_MS)) {
        setSensorPowerEnabled(true);
        delay(50);
        beginAds1115();
        const SensorReadings sensors = readSensors();
        const GpsFix gps = readGpsFix();
        const int rssi = readRssi();
        publishTelemetry(buildTelemetryPayload(sensors, gps, rssi));
        setSensorPowerEnabled(false);
        lastPublish = now;
    }

    delay(10);
}

#ifndef TINY_GSM_FORK_LIBRARY
#error "Please install the LilyGo TinyGSM fork from LilyGo-Modem-Series/lib."
#endif
