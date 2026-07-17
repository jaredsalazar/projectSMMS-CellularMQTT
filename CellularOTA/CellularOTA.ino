#include "Config.h"
#include "utilities.h"
#include "BoardPower.h"
#include "Sensors.h"
#include "ModemMqtt.h"
#include "Gps.h"
#include "Telemetry.h"
#include "CellularOta.h"
#include "esp_sleep.h"

bool modemReady = false;

bool waitForMqttConnection()
{
    const uint32_t started = millis();
    while (!mqtt.connected() && millis() - started < MQTT_CONNECT_WAIT_MS) {
        maintainConnections(mqttClientId);
        delay(100);
    }
    return mqtt.connected();
}

bool publishOnce()
{
    if (!waitForMqttConnection()) {
        SerialMon.println("MQTT connect timed out; sleeping and retrying next cycle.");
        return false;
    }

    const SensorReadings sensors = readSensors();
    const GpsFix gps = readGpsFix();
    const int rssi = readRssi();
    const bool published = publishTelemetry(buildTelemetryPayload(sensors, gps, rssi));
    mqtt.loop();
    delay(250);
    return published;
}

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
    SerialMon.println("SMMS Cellular MQTT starting - Cellular OTA sleep mode");
    SerialMon.println(PRODUCT_MODEL_NAME);
    SerialMon.print("Firmware version: ");
    SerialMon.println(FIRMWARE_VERSION);

    buildDeviceIdentity();
    SerialMon.print("Device ID: ");
    SerialMon.println(deviceId);

    configureBoardPins();
    holdBatteryPowerRail();
    setSensorPowerEnabled(true);
    delay(50);
    configureAnalogInputs();
    beginAds1115();

    powerOnModem();
    modemReady = initializeModem();
    if (modemReady) {
        ensureNetworkConnection(true);
        ensureGprsConnection();
        checkForCellularOta();
        initializeGps();
        publishOnce();
    } else {
        SerialMon.println("Modem init failed; sleeping and retrying next cycle.");
    }

    sleepUntilNextSend();
}

void loop()
{
    sleepUntilNextSend();
}

#ifndef TINY_GSM_FORK_LIBRARY
#error "Please install the LilyGo TinyGSM fork from LilyGo-Modem-Series/lib."
#endif
