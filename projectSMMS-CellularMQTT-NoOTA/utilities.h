#pragma once

// LilyGo T-A7670 / A7670SA ESP32 board profile.
#define LILYGO_T_A7670

#if defined(LILYGO_T_A7670)
#define MODEM_BAUDRATE                  (115200)
#define MODEM_DTR_PIN                   (25)
#define MODEM_TX_PIN                    (26)
#define MODEM_RX_PIN                    (27)
#define BOARD_PWRKEY_PIN                (4)
#define BOARD_POWERON_PIN               (12)
#define MODEM_RING_PIN                  (33)
#define MODEM_RESET_PIN                 (5)
#define BOARD_BAT_ADC_PIN               (35)
#define BOARD_SOLAR_ADC_PIN             (36)
#define BOARD_SENSOR_POWER_EN_PIN       (32)
#define BOARD_SENSOR_POWER_EN_LEVEL     HIGH
#define MODEM_RESET_LEVEL               HIGH
#define BOARD_SDA_PIN                   (21)
#define BOARD_SCL_PIN                   (22)
#define MODEM_GPS_ENABLE_GPIO           (-1)
#define MODEM_GPS_ENABLE_LEVEL          (-1)
#define PRODUCT_MODEL_NAME              "LilyGo-T-A7670"
#define SerialAT                        Serial1

#ifndef TINY_GSM_MODEM_A7670
#define TINY_GSM_MODEM_A7670
#endif
#else
#error "This sketch is configured for LilyGo T-A7670 / A7670SA ESP32."
#endif

#if defined(TINY_GSM_MODEM_A7670)
#define MODEM_POWERON_PULSE_WIDTH_MS    (100)
#define MODEM_POWEROFF_PULSE_WIDTH_MS   (3000)
#define MODEM_REG_SMS_ONLY
#endif

#ifndef MODEM_START_WAIT_MS
#define MODEM_START_WAIT_MS             (3000)
#endif
