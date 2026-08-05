#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#define SerialMon Serial

const uint32_t SERIAL_BAUD = 115200;
const uint32_t WRITE_INTERVAL_MS = 10000UL;
const uint32_t SD_RETRY_INTERVAL_MS = 5000UL;

const uint8_t BOARD_POWERON_PIN = 12;
const uint8_t BOARD_SD_SCK_PIN = 14;
const uint8_t BOARD_SD_MISO_PIN = 2;
const uint8_t BOARD_SD_MOSI_PIN = 15;
const uint8_t BOARD_SD_CS_PIN = 13;

const char SD_TEST_FILE[] = "/sd_test.csv";
const char SD_TEST_SENTENCE[] = "this is an sd card test";

uint32_t lastWriteMs = 0;
uint32_t lastSdRetryMs = 0;
bool sdReady = false;

void enableBoardPowerRail()
{
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);

    SerialMon.print("Board power rail GPIO ");
    SerialMon.print(BOARD_POWERON_PIN);
    SerialMon.println(" enabled.");
}

bool beginSdCard()
{
    SerialMon.println("Enabling TTGO A7670SA board power rail...");
    enableBoardPowerRail();
    delay(500);

    SerialMon.print("Starting SPI with SCK=");
    SerialMon.print(BOARD_SD_SCK_PIN);
    SerialMon.print(", MISO=");
    SerialMon.print(BOARD_SD_MISO_PIN);
    SerialMon.print(", MOSI=");
    SerialMon.print(BOARD_SD_MOSI_PIN);
    SerialMon.print(", CS=");
    SerialMon.println(BOARD_SD_CS_PIN);
    SPI.begin(BOARD_SD_SCK_PIN, BOARD_SD_MISO_PIN, BOARD_SD_MOSI_PIN, BOARD_SD_CS_PIN);

    SerialMon.println("Mounting SD card...");
    SD.end();
    if (!SD.begin(BOARD_SD_CS_PIN, SPI)) {
        SerialMon.println("SD card init failed.");
        SerialMon.println("Will retry. Check card is inserted, formatted FAT32, and the built-in SD slot is being used.");
        return false;
    }
    SerialMon.println("SD card mounted successfully.");

    const uint8_t cardType = SD.cardType();
    SerialMon.print("Detected SD card type: ");
    if (cardType == CARD_MMC) {
        SerialMon.println("MMC");
    } else if (cardType == CARD_SD) {
        SerialMon.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        SerialMon.println("SDHC/SDXC");
    } else {
        SerialMon.println("unknown");
    }
    SerialMon.print("Detected SD card size: ");
    SerialMon.print(SD.cardSize() / (1024ULL * 1024ULL));
    SerialMon.println(" MB");

    const bool needsHeader = !SD.exists(SD_TEST_FILE);
    SerialMon.print("Opening CSV file ");
    SerialMon.print(SD_TEST_FILE);
    SerialMon.println(needsHeader ? " and creating header." : " for append.");

    File testFile = SD.open(SD_TEST_FILE, FILE_APPEND);
    if (!testFile) {
        SerialMon.print("Could not open ");
        SerialMon.println(SD_TEST_FILE);
        return false;
    }

    if (needsHeader) {
        SerialMon.println("Writing CSV header: uptime_ms,message");
        testFile.println("uptime_ms,message");
    }

    testFile.close();
    SerialMon.println("CSV file closed after setup.");
    SerialMon.print("Writing SD test rows to ");
    SerialMon.println(SD_TEST_FILE);
    return true;
}

bool appendSdTestRow()
{
    if (!sdReady) {
        SerialMon.println("SD card is not ready.");
        return false;
    }

    SerialMon.print("Opening ");
    SerialMon.print(SD_TEST_FILE);
    SerialMon.println(" for append...");
    File testFile = SD.open(SD_TEST_FILE, FILE_APPEND);
    if (!testFile) {
        SerialMon.print("Could not append to ");
        SerialMon.println(SD_TEST_FILE);
        return false;
    }

    SerialMon.print("Appending CSV row at uptime ");
    SerialMon.print(millis());
    SerialMon.println(" ms.");
    testFile.printf("%lu,%s\n", static_cast<unsigned long>(millis()), SD_TEST_SENTENCE);
    testFile.close();

    SerialMon.print("Wrote: ");
    SerialMon.println(SD_TEST_SENTENCE);
    SerialMon.println("CSV file closed after append.");
    return true;
}

void setup()
{
    SerialMon.begin(SERIAL_BAUD);
    delay(500);

    SerialMon.println();
    SerialMon.println("SD card CSV write test starting.");
    SerialMon.print("Write interval: ");
    SerialMon.print(WRITE_INTERVAL_MS / 1000UL);
    SerialMon.println(" seconds.");
    SerialMon.print("SD retry interval: ");
    SerialMon.print(SD_RETRY_INTERVAL_MS / 1000UL);
    SerialMon.println(" seconds.");
    SerialMon.print("Test sentence: ");
    SerialMon.println(SD_TEST_SENTENCE);

    sdReady = beginSdCard();

    if (sdReady) {
        SerialMon.println("Writing first test row now.");
        appendSdTestRow();
        lastWriteMs = millis();
        SerialMon.println("Next test row will be written in 10 seconds.");
    } else {
        lastSdRetryMs = millis();
        SerialMon.println("Setup finished with SD unavailable. The sketch will keep retrying.");
    }
}

void loop()
{
    if (!sdReady && millis() - lastSdRetryMs >= SD_RETRY_INTERVAL_MS) {
        SerialMon.println("Retrying SD card setup...");
        sdReady = beginSdCard();
        lastSdRetryMs = millis();

        if (sdReady) {
            SerialMon.println("SD card is ready after retry. Writing first test row now.");
            appendSdTestRow();
            lastWriteMs = millis();
            SerialMon.println("Next test row will be written in 10 seconds.");
        }
        return;
    }

    if (sdReady && millis() - lastWriteMs >= WRITE_INTERVAL_MS) {
        SerialMon.println("10 seconds elapsed. Writing another SD test row.");
        appendSdTestRow();
        lastWriteMs = millis();
        SerialMon.println("Waiting 10 seconds before the next write.");
    }
}
