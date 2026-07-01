#pragma once

#include <Arduino.h>

#define SerialMon Serial
#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

const uint32_t SERIAL_BAUD = 115200;
const uint32_t NETWORK_TIMEOUT_MS = 180000UL;
const uint32_t MQTT_RECONNECT_INTERVAL_MS = 10000UL;
const uint32_t PUBLISH_INTERVAL_MS = 15000UL;
const uint32_t GPS_REFRESH_INTERVAL_MS = 15000UL;
const uint32_t MQTT_CONNECT_WAIT_MS = 60000UL;
const uint64_t DEEP_SLEEP_INTERVAL_US = 5ULL * 60ULL * 1000000ULL;

const char GSM_PIN[] = "";
const char APN[] = "internet";
const char GPRS_USER[] = "";
const char GPRS_PASS[] = "";

const char MQTT_BROKER[] = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char MQTT_TOPIC[] = "sapat/test";
const size_t MQTT_BUFFER_SIZE = 1024;
const size_t TELEMETRY_PAYLOAD_SIZE = 768;

const uint8_t ADC_SAMPLE_COUNT = 8;
const float BATTERY_DIVIDER_RATIO = 2.0f;
const float SOLAR_DIVIDER_RATIO = 2.0f;
const uint16_t BATTERY_EMPTY_MV = 3300;
const uint16_t BATTERY_FULL_MV = 4200;
const uint16_t SOLAR_CHARGING_MARGIN_MV = 100;

const uint8_t ADS1115_DEFAULT_ADDRESS = 0x48;
const uint8_t ADS1115_CONFIG_REGISTER = 0x01;
const uint8_t ADS1115_CONVERSION_REGISTER = 0x00;
const uint16_t ADS1115_SINGLE_SHOT = 0x8000;
const uint16_t ADS1115_PGA_4096 = 0x0200;
const uint16_t ADS1115_SINGLE_SHOT_MODE = 0x0100;
const uint16_t ADS1115_DATA_RATE_128_SPS = 0x0080;
const uint16_t ADS1115_COMPARATOR_DISABLED = 0x0003;
const float ADS1115_VOLTS_PER_BIT = 0.000125f;
