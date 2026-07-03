#pragma once

#include <Arduino.h>
#include <Wire.h>
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

const uint16_t ADS1115_MUX_SINGLE_ENDED[4] = {
    0x4000,
    0x5000,
    0x6000,
    0x7000,
};

bool writeAds1115Register(uint8_t reg, uint16_t value)
{
    Wire.beginTransmission(ads1115Address);
    Wire.write(reg);
    Wire.write(static_cast<uint8_t>(value >> 8));
    Wire.write(static_cast<uint8_t>(value & 0xFF));
    return Wire.endTransmission() == 0;
}

bool readAds1115Register(uint8_t reg, uint16_t *value)
{
    Wire.beginTransmission(ads1115Address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(ads1115Address, static_cast<uint8_t>(2)) != 2) {
        return false;
    }

    *value = (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read();
    return true;
}

bool scanAds1115Address()
{
    SerialMon.println("Scanning I2C bus...");

    bool foundAds1115 = false;
    for (uint8_t address = 0x48; address <= 0x4B; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            ads1115Address = address;
            foundAds1115 = true;
            break;
        }
        delay(2);
    }

    if (foundAds1115) {
        SerialMon.print("ADS1115 detected at 0x");
        SerialMon.println(ads1115Address, HEX);
    } else {
        SerialMon.println("ADS1115 not found at 0x48-0x4B.");
    }

    return foundAds1115;
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

    if (!scanAds1115Address()) {
        ads1115Ready = false;
        return false;
    }

    uint16_t config = 0;
    ads1115Ready = readAds1115Register(ADS1115_CONFIG_REGISTER, &config);
    if (ads1115Ready) {
        SerialMon.println("ADS1115 register read OK.");
    } else {
        SerialMon.println("ADS1115 register read failed.");
    }
    return ads1115Ready;
}

bool beginAds1115()
{
    if (beginAds1115OnPins(BOARD_SDA_PIN, BOARD_SCL_PIN)) {
        return true;
    }

    const int fallbackPairs[][2] = {
        {21, 22},
        {22, 21},
        {17, 18},
        {18, 17},
    };

    for (uint8_t i = 0; i < sizeof(fallbackPairs) / sizeof(fallbackPairs[0]); ++i) {
        if (fallbackPairs[i][0] == BOARD_SDA_PIN && fallbackPairs[i][1] == BOARD_SCL_PIN) {
            continue;
        }
        if (beginAds1115OnPins(fallbackPairs[i][0], fallbackPairs[i][1])) {
            return true;
        }
    }

    return false;
}

bool readAds1115SingleEnded(uint8_t channel, AnalogChannelReading *reading)
{
    if (reading == nullptr || !ads1115Ready || channel > 3) {
        return false;
    }

    const uint16_t config = ADS1115_SINGLE_SHOT |
                            ADS1115_MUX_SINGLE_ENDED[channel] |
                            ADS1115_PGA_4096 |
                            ADS1115_SINGLE_SHOT_MODE |
                            ADS1115_DATA_RATE_128_SPS |
                            ADS1115_COMPARATOR_DISABLED;

    if (!writeAds1115Register(ADS1115_CONFIG_REGISTER, config)) {
        return false;
    }

    uint16_t status = 0;
    const uint32_t started = millis();
    do {
        delay(2);
        if (!readAds1115Register(ADS1115_CONFIG_REGISTER, &status)) {
            return false;
        }
    } while ((status & 0x8000) == 0 && millis() - started < 50);

    uint16_t raw = 0;
    if (!readAds1115Register(ADS1115_CONVERSION_REGISTER, &raw)) {
        return false;
    }

    reading->raw = static_cast<int16_t>(raw);
    reading->voltage = reading->raw * ADS1115_VOLTS_PER_BIT;
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
