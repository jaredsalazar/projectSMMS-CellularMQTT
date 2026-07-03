#pragma once

#include <Arduino.h>
#include <Update.h>
#include "Config.h"
#include "ModemMqtt.h"
#include "Sensors.h"

TinyGsmClient otaClient(modem);

int compareVersionPart(const char **left, const char **right)
{
    unsigned long leftValue = 0;
    unsigned long rightValue = 0;

    while (**left >= '0' && **left <= '9') {
        leftValue = (leftValue * 10UL) + static_cast<unsigned long>(**left - '0');
        ++(*left);
    }

    while (**right >= '0' && **right <= '9') {
        rightValue = (rightValue * 10UL) + static_cast<unsigned long>(**right - '0');
        ++(*right);
    }

    if (**left == '.') {
        ++(*left);
    }
    if (**right == '.') {
        ++(*right);
    }

    if (leftValue > rightValue) {
        return 1;
    }
    if (leftValue < rightValue) {
        return -1;
    }
    return 0;
}

int compareVersions(const char *left, const char *right)
{
    while (*left != '\0' || *right != '\0') {
        const int part = compareVersionPart(&left, &right);
        if (part != 0) {
            return part;
        }

        while (*left != '\0' && *left != '.' && (*left < '0' || *left > '9')) {
            ++left;
        }
        while (*right != '\0' && *right != '.' && (*right < '0' || *right > '9')) {
            ++right;
        }
    }

    return 0;
}

bool otaReadLine(String &line, uint32_t timeoutMs)
{
    line = "";
    const uint32_t started = millis();

    while (millis() - started < timeoutMs) {
        while (otaClient.available()) {
            const char c = otaClient.read();
            if (c == '\n') {
                line.trim();
                return true;
            }
            if (c != '\r') {
                line += c;
            }
        }
        delay(5);
    }

    return false;
}

bool otaSendGetRequest(const char *path)
{
    otaClient.stop();
    SerialMon.print("OTA connecting to ");
    SerialMon.print(OTA_HOST);
    SerialMon.print(":");
    SerialMon.println(OTA_PORT);

    if (!otaClient.connect(OTA_HOST, OTA_PORT)) {
        SerialMon.println("OTA server connect failed.");
        return false;
    }

    otaClient.print("GET ");
    otaClient.print(path);
    otaClient.println(" HTTP/1.1");
    otaClient.print("Host: ");
    otaClient.println(OTA_HOST);
    otaClient.println("User-Agent: projectSMMS-CellularOTA");
    otaClient.println("Connection: close");
    otaClient.println();
    return true;
}

bool otaReadHttpHeaders(int *statusCode, int *contentLength)
{
    String line;
    if (!otaReadLine(line, OTA_CONNECT_TIMEOUT_MS)) {
        SerialMon.println("OTA HTTP response timeout.");
        return false;
    }

    const int firstSpace = line.indexOf(' ');
    *statusCode = firstSpace > 0 ? line.substring(firstSpace + 1).toInt() : 0;
    *contentLength = -1;

    while (otaReadLine(line, OTA_CONNECT_TIMEOUT_MS)) {
        if (line.length() == 0) {
            return true;
        }

        String lower = line;
        lower.toLowerCase();
        if (lower.startsWith("content-length:")) {
            *contentLength = line.substring(line.indexOf(':') + 1).toInt();
        }
    }

    return false;
}

bool otaFetchLatestVersion(char *latestVersion, size_t latestVersionSize)
{
    if (!otaSendGetRequest(OTA_VERSION_PATH)) {
        return false;
    }

    int statusCode = 0;
    int contentLength = -1;
    if (!otaReadHttpHeaders(&statusCode, &contentLength) || statusCode != 200) {
        SerialMon.print("OTA version HTTP status ");
        SerialMon.println(statusCode);
        otaClient.stop();
        return false;
    }

    size_t index = 0;
    const uint32_t started = millis();
    while (millis() - started < OTA_CONNECT_TIMEOUT_MS && index < latestVersionSize - 1) {
        while (otaClient.available() && index < latestVersionSize - 1) {
            const char c = otaClient.read();
            if (c == '\n' || c == '\r') {
                latestVersion[index] = '\0';
                otaClient.stop();
                return index > 0;
            }
            latestVersion[index++] = c;
        }
        if (!otaClient.connected() && !otaClient.available()) {
            break;
        }
        delay(5);
    }

    latestVersion[index] = '\0';
    otaClient.stop();
    return index > 0;
}

bool otaDownloadAndApply()
{
    if (!otaSendGetRequest(OTA_BINARY_PATH)) {
        return false;
    }

    int statusCode = 0;
    int contentLength = -1;
    if (!otaReadHttpHeaders(&statusCode, &contentLength) || statusCode != 200 || contentLength <= 0) {
        SerialMon.print("OTA binary HTTP status ");
        SerialMon.print(statusCode);
        SerialMon.print(", size ");
        SerialMon.println(contentLength);
        otaClient.stop();
        return false;
    }

    SerialMon.print("OTA binary size: ");
    SerialMon.println(contentLength);

    if (!Update.begin(contentLength)) {
        SerialMon.print("OTA Update.begin failed: ");
        SerialMon.println(Update.errorString());
        otaClient.stop();
        return false;
    }

    uint8_t buffer[512];
    int written = 0;
    uint32_t lastRead = millis();

    while (written < contentLength && millis() - lastRead < OTA_READ_TIMEOUT_MS) {
        const int available = otaClient.available();
        if (available > 0) {
            const int toRead = min(available, static_cast<int>(sizeof(buffer)));
            const int readBytes = otaClient.readBytes(buffer, toRead);
            if (readBytes > 0) {
                const size_t flashed = Update.write(buffer, readBytes);
                if (flashed != static_cast<size_t>(readBytes)) {
                    SerialMon.print("OTA flash write failed: ");
                    SerialMon.println(Update.errorString());
                    Update.abort();
                    otaClient.stop();
                    return false;
                }
                written += readBytes;
                lastRead = millis();
            }
        } else {
            delay(5);
        }
    }

    otaClient.stop();

    if (written != contentLength) {
        SerialMon.print("OTA download incomplete: ");
        SerialMon.print(written);
        SerialMon.print("/");
        SerialMon.println(contentLength);
        Update.abort();
        return false;
    }

    if (!Update.end(true)) {
        SerialMon.print("OTA Update.end failed: ");
        SerialMon.println(Update.errorString());
        return false;
    }

    SerialMon.println("OTA update complete. Restarting.");
    SerialMon.flush();
    ESP.restart();
    return true;
}

bool checkForCellularOta()
{
    if (!OTA_ENABLED) {
        return false;
    }

    SensorReadings sensors = readSensors();
    const int rssi = readRssi();

    if (sensors.batteryPercent < OTA_MIN_BATTERY_PERCENT) {
        SerialMon.print("OTA skipped, battery too low: ");
        SerialMon.print(sensors.batteryPercent);
        SerialMon.println("%");
        return false;
    }

    if (rssi != -999 && rssi < OTA_MIN_RSSI) {
        SerialMon.print("OTA skipped, RSSI too low: ");
        SerialMon.println(rssi);
        return false;
    }

    char latestVersion[24] = {};
    if (!otaFetchLatestVersion(latestVersion, sizeof(latestVersion))) {
        SerialMon.println("OTA version check failed.");
        return false;
    }

    SerialMon.print("Current firmware: ");
    SerialMon.println(FIRMWARE_VERSION);
    SerialMon.print("Latest firmware: ");
    SerialMon.println(latestVersion);

    if (compareVersions(latestVersion, FIRMWARE_VERSION) <= 0) {
        SerialMon.println("OTA not needed.");
        return false;
    }

    SerialMon.println("OTA update available.");
    return otaDownloadAndApply();
}
