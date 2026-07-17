#pragma once

#include <Arduino.h>
#include "driver/gpio.h"
#include "Config.h"
#include "utilities.h"

void configureBoardPins()
{
#ifdef BOARD_POWERON_PIN
    gpio_hold_dis(static_cast<gpio_num_t>(BOARD_POWERON_PIN));
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    SerialMon.print("Battery power rail enabled on GPIO ");
    SerialMon.println(BOARD_POWERON_PIN);
#endif

#ifdef BOARD_SENSOR_POWER_EN_PIN
    gpio_hold_dis(static_cast<gpio_num_t>(BOARD_SENSOR_POWER_EN_PIN));
    pinMode(BOARD_SENSOR_POWER_EN_PIN, OUTPUT);
    digitalWrite(BOARD_SENSOR_POWER_EN_PIN, !BOARD_SENSOR_POWER_EN_LEVEL);
    SerialMon.print("Sensor power enable configured on GPIO ");
    SerialMon.println(BOARD_SENSOR_POWER_EN_PIN);
#endif

#ifdef MODEM_RESET_PIN
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    delay(100);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
    delay(2600);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
#endif

#ifdef MODEM_DTR_PIN
    pinMode(MODEM_DTR_PIN, OUTPUT);
    digitalWrite(MODEM_DTR_PIN, LOW);
#endif

    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
}

void holdBatteryPowerRail()
{
#ifdef BOARD_POWERON_PIN
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    gpio_hold_en(static_cast<gpio_num_t>(BOARD_POWERON_PIN));
    gpio_deep_sleep_hold_en();
#endif
}

void setSensorPowerEnabled(bool enabled)
{
#ifdef BOARD_SENSOR_POWER_EN_PIN
    gpio_hold_dis(static_cast<gpio_num_t>(BOARD_SENSOR_POWER_EN_PIN));
    digitalWrite(BOARD_SENSOR_POWER_EN_PIN, enabled ? BOARD_SENSOR_POWER_EN_LEVEL : !BOARD_SENSOR_POWER_EN_LEVEL);
#endif
}

void holdSensorPowerOffDuringSleep()
{
#ifdef BOARD_SENSOR_POWER_EN_PIN
    digitalWrite(BOARD_SENSOR_POWER_EN_PIN, !BOARD_SENSOR_POWER_EN_LEVEL);
    gpio_hold_en(static_cast<gpio_num_t>(BOARD_SENSOR_POWER_EN_PIN));
    gpio_deep_sleep_hold_en();
#endif
}

void powerOnModem()
{
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(MODEM_POWERON_PULSE_WIDTH_MS);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(250);
}
