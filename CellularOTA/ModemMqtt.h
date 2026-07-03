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

    if (strlen(GSM_PIN) > 0 && modem.getSimStatus() != 3) {
        modem.simUnlock(GSM_PIN);
    }

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
