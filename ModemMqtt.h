#pragma once

#include <Arduino.h>
#include <string.h>
#include "Config.h"
#include "utilities.h"

#define TINY_GSM_DEBUG SerialMon
#include <TinyGsmClient.h>
#include <PubSubClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient netClient(modem);
PubSubClient mqtt(netClient);

uint32_t lastReconnectAttempt = 0;

bool initializeModem()
{
    // SerialAT is the ESP32 UART connected to the cellular modem.
    SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

    SerialMon.println("Starting modem...");
    delay(MODEM_START_WAIT_MS);

    if (!modem.init()) {
        SerialMon.println("modem.init() failed");
        return false;
    }

    SerialMon.print("Modem Name: ");
    SerialMon.println(modem.getModemName());
    SerialMon.print("Modem Info: ");
    SerialMon.println(modem.getModemInfo());

    // Unlock SIMs that require a PIN; blank GSM_PIN means no unlock is needed.
    if (strlen(GSM_PIN) > 0 && modem.getSimStatus() != 3) {
        modem.simUnlock(GSM_PIN);
    }

    // PubSubClient settings must be sized before publishing the JSON payload.
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setKeepAlive(30);
    mqtt.setSocketTimeout(30);
    mqtt.setBufferSize(MQTT_BUFFER_SIZE);

    return true;
}

bool ensureNetworkConnection(bool forceWait = false)
{
    if (!forceWait && modem.isNetworkConnected()) {
        return true;
    }

    // Network registration means the modem has attached to the cellular tower.
    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork(NETWORK_TIMEOUT_MS, true)) {
        SerialMon.println(" fail");
        return false;
    }

    SerialMon.println(" success");
    return true;
}

bool ensureGprsConnection()
{
    if (modem.isGprsConnected()) {
        return true;
    }

    // GPRS/APN creates the IP data session used by MQTT.
    SerialMon.print("Connecting to APN ");
    SerialMon.print(APN);
    SerialMon.print(" ... ");
    if (!modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
        SerialMon.println("fail");
        return false;
    }

    SerialMon.println("success");
    SerialMon.print("IP: ");
    SerialMon.println(modem.localIP());
    return true;
}

bool ensureDataConnection()
{
    return ensureNetworkConnection() && ensureGprsConnection();
}

bool mqttConnect(const char *clientId)
{
    SerialMon.print("Connecting to MQTT ");
    SerialMon.print(MQTT_BROKER);
    SerialMon.print(":");
    SerialMon.print(MQTT_PORT);
    SerialMon.print(" ... ");

    const bool ok = mqtt.connect(clientId);
    if (!ok) {
        SerialMon.print("failed, rc=");
        SerialMon.println(mqtt.state());
        return false;
    }

    SerialMon.println("success");
    return true;
}

void maintainConnections(const char *clientId)
{
    if (!ensureDataConnection()) {
        return;
    }

    if (mqtt.connected()) {
        mqtt.loop();
        return;
    }

    // Reconnect attempts are rate-limited so a bad signal does not spin forever.
    const uint32_t now = millis();
    if (lastReconnectAttempt == 0 || now - lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL_MS) {
        lastReconnectAttempt = now;
        if (mqttConnect(clientId)) {
            lastReconnectAttempt = 0;
        }
    }
}

int signalQualityToRssi(int16_t csq)
{
    // TinyGSM returns CSQ 0-31; convert it to approximate RSSI in dBm.
    if (csq < 0 || csq == 99 || csq > 31) {
        return -999;
    }
    return -113 + (2 * csq);
}

int readRssi()
{
    return signalQualityToRssi(modem.getSignalQuality());
}

bool publishTelemetry(const char *payload)
{
    const bool ok = mqtt.publish(MQTT_TOPIC, payload);
    SerialMon.print(ok ? "Published " : "Publish failed ");
    SerialMon.print(MQTT_TOPIC);
    SerialMon.print(" -> ");
    SerialMon.println(payload);
    return ok;
}
