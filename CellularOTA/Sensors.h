#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "Config.h"
#include "utilities.h"

struct AnalogChannelReading {
    int16_t raw;
    float voltage;
    bool ok;
};

struct SensorReadings {
    uint32_t batteryMilliVolts;
    uint8_t batteryPercent;
    uint32_t solarMilliVolts;
    bool solarCharging;
    bool adsOk;
    AnalogChannelReading analog[4];
};

uint8_t ads1115Address = ADS1115_DEFAULT_ADDRESS;
bool ads1115Ready = false;
int activeSdaPin = -1;
int activeSclPin = -1;
Adafruit_ADS1115 ads;

bool probeAds1115Address()
{
    ads1115Address = ADS1115_DEFAULT_ADDRESS;
    SerialMon.print("Checking ADS1115 at fixed address 0x");
    SerialMon.println(ads1115Address, HEX);

    Wire.beginTransmission(ads1115Address);
    if (Wire.endTransmission() == 0) {
        SerialMon.print("ADS1115 detected at 0x");
        SerialMon.println(ads1115Address, HEX);
        return true;
    }

    SerialMon.println("ADS1115 not found at 0x48. Confirm ADDR is tied to GND, SDA is GPIO21, and SCL is GPIO22.");
    return false;
}

bool beginAds1115OnPins(int sdaPin, int sclPin)
{
    Wire.end();
    delay(50);
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(100000);

    activeSdaPin = sdaPin;
    activeSclPin = sclPin;

    SerialMon.print("Trying I2C SDA ");
    SerialMon.print(activeSdaPin);
    SerialMon.print(", SCL ");
    SerialMon.println(activeSclPin);

    if (!probeAds1115Address()) {
        ads1115Ready = false;
        return false;
    }

    ads.setGain(GAIN_TWOTHIRDS);
    ads1115Ready = ads.begin(ads1115Address, &Wire);
    if (ads1115Ready) {
        SerialMon.println("ADS1115 Adafruit driver started.");
    } else {
        SerialMon.println("ADS1115 Adafruit driver start failed.");
    }
    return ads1115Ready;
}

bool beginAds1115()
{
    return beginAds1115OnPins(BOARD_SDA_PIN, BOARD_SCL_PIN);
}

bool readAds1115SingleEnded(uint8_t channel, AnalogChannelReading *reading)
{
    if (reading == nullptr || !ads1115Ready || channel > 3) {
        return false;
    }

    reading->raw = ads.readADC_SingleEnded(channel);
    reading->voltage = ads.computeVolts(reading->raw);
    reading->ok = true;
    return true;
}

void configureAnalogInputs()
{
    pinMode(BOARD_BAT_ADC_PIN, INPUT);
    pinMode(BOARD_SOLAR_ADC_PIN, INPUT);

#if defined(ESP32)
    analogReadResolution(12);
    analogSetPinAttenuation(BOARD_BAT_ADC_PIN, ADC_11db);
    analogSetPinAttenuation(BOARD_SOLAR_ADC_PIN, ADC_11db);
#endif
}

uint32_t readScaledMilliVolts(uint8_t pin, float dividerRatio)
{
#if defined(ESP32)
    uint32_t totalMilliVolts = 0;
    for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; ++i) {
        totalMilliVolts += analogReadMilliVolts(pin);
        delay(5);
    }

    const float averageMilliVolts = totalMilliVolts / static_cast<float>(ADC_SAMPLE_COUNT);
    return static_cast<uint32_t>(averageMilliVolts * dividerRatio + 0.5f);
#else
    return 0;
#endif
}

uint8_t batteryPercentFromMilliVolts(uint32_t milliVolts)
{
    if (milliVolts <= BATTERY_EMPTY_MV) {
        return 0;
    }
    if (milliVolts >= BATTERY_FULL_MV) {
        return 100;
    }
    return static_cast<uint8_t>(((milliVolts - BATTERY_EMPTY_MV) * 100UL) /
                                (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
}

SensorReadings readSensors()
{
    SensorReadings readings = {};
    readings.batteryMilliVolts = readScaledMilliVolts(BOARD_BAT_ADC_PIN, BATTERY_DIVIDER_RATIO);
    readings.batteryPercent = batteryPercentFromMilliVolts(readings.batteryMilliVolts);
    readings.solarMilliVolts = readScaledMilliVolts(BOARD_SOLAR_ADC_PIN, SOLAR_DIVIDER_RATIO);
    readings.solarCharging = readings.solarMilliVolts > readings.batteryMilliVolts + SOLAR_CHARGING_MARGIN_MV;
    readings.adsOk = ads1115Ready;

    if (!ads1115Ready) {
        SerialMon.println("ADS1115 not ready. Retrying scan...");
        readings.adsOk = beginAds1115();
    }

    for (uint8_t channel = 0; channel < 4; ++channel) {
        readings.analog[channel].ok = false;
        if (!readAds1115SingleEnded(channel, &readings.analog[channel])) {
            readings.adsOk = false;
            SerialMon.print("ADS1115 read failed on channel ");
            SerialMon.println(channel);
        }
    }

    return readings;
}
