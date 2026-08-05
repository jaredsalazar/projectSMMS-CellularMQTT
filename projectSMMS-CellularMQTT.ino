#include "Config.h"
#include "utilities.h"
#include "BoardPower.h"
#include "Sensors.h"
#include "ModemMqtt.h"
#include "Gps.h"
#include "Telemetry.h"
#include "DataLogger.h"
#include "esp_sleep.h"

bool modemReady = false;

// Waits for MQTT while also keeping the cellular data connection alive.
bool waitForMqttConnection()
{
    const uint32_t started = millis();
    while (!mqtt.connected() && millis() - started < MQTT_CONNECT_WAIT_MS) {
        maintainConnections(mqttClientId);
        delay(100);
    }
    return mqtt.connected();
}

// One wake cycle publishes exactly one telemetry message, then the board sleeps.
bool publishOnce()
{
    const SensorReadings sensors = readSensors();
    const GpsFix gps = modemReady ? readGpsFix() : lastGpsFix;
    const int rssi = modemReady ? readRssi() : -999;
    const char *payload = buildTelemetryPayload(sensors, gps, rssi);

    // Keep the sensor rail HIGH during SD writes in case the SD module shares it.
    setSensorPowerEnabled(true);
    delay(20);
    appendMeasurementToSd(sensors, gps, rssi);
    setSensorPowerEnabled(false);

    if (!modemReady) {
        SerialMon.println("Modem not ready; measurement was logged but not sent.");
        return false;
    }

    if (!waitForMqttConnection()) {
        SerialMon.println("MQTT connect timed out; measurement was logged and will retry sending next cycle.");
        return false;
    }

    const bool published = publishTelemetry(payload);
    mqtt.loop();
    delay(250);
    return published;
}

// Turns off the high-power parts and asks the ESP32 to wake itself in 5 minutes.
void sleepUntilNextSend()
{
    SerialMon.println("Preparing for 5 minute deep sleep.");
    mqtt.disconnect();

    if (modemReady) {
        modem.gprsDisconnect();
        modem.poweroff();
    }

    setSensorPowerEnabled(false);
    holdSensorPowerOffDuringSleep();
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_INTERVAL_US);
    SerialMon.flush();
    esp_deep_sleep_start();
}

void setup()
{
    SerialMon.begin(SERIAL_BAUD);
    delay(500);

    SerialMon.println();
    SerialMon.println("SMMS Cellular MQTT starting - 5min sleep mode");
    SerialMon.println(PRODUCT_MODEL_NAME);
    SerialMon.print("Firmware version: ");
    SerialMon.println(FIRMWARE_VERSION);

    // Use the ESP32 chip ID so every board gets a stable MQTT/device identity.
    buildDeviceIdentity();
    SerialMon.print("Device ID: ");
    SerialMon.println(deviceId);

    // Bring up board rails first, then sensors, then the modem/network stack.
    configureBoardPins();
    holdBatteryPowerRail();
    configureAnalogInputs();
    setSensorPowerEnabled(true);
    delay(50);
    beginSdLogger();
    beginAds1115();
    setSensorPowerEnabled(false);

    powerOnModem();
    modemReady = initializeModem();
    if (modemReady) {
        ensureNetworkConnection(true);
        ensureGprsConnection();
        initializeGps();
    } else {
        SerialMon.println("Modem init failed; sleeping and retrying next cycle.");
    }

    publishOnce();
    sleepUntilNextSend();
}

void loop()
{
    sleepUntilNextSend();
}

#ifndef TINY_GSM_FORK_LIBRARY
#error "Please install the LilyGo TinyGSM fork from LilyGo-Modem-Series/lib."
#endif
