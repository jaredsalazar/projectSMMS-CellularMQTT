#include <Arduino.h>
#include <Wire.h>

#define SerialMon Serial

const uint32_t SERIAL_BAUD = 115200;
const uint8_t BOARD_POWERON_PIN = 12;
const uint8_t BOARD_SENSOR_POWER_EN_PIN = 19;
const uint8_t BOARD_SDA_PIN = 21;
const uint8_t BOARD_SCL_PIN = 22;

void enableBoardPowerRail()
{
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
    SerialMon.print("Board power rail GPIO ");
    SerialMon.print(BOARD_POWERON_PIN);
    SerialMon.println(" HIGH.");
}

void scanI2cBus(const char *label)
{
    SerialMon.print("Scanning I2C bus with ");
    SerialMon.println(label);

    uint8_t foundCount = 0;
    for (uint8_t address = 1; address < 127; ++address) {
        Wire.beginTransmission(address);
        const uint8_t error = Wire.endTransmission();
        if (error == 0) {
            SerialMon.print("Found I2C device at 0x");
            if (address < 16) {
                SerialMon.print("0");
            }
            SerialMon.println(address, HEX);
            foundCount++;
        }
        delay(2);
    }

    if (foundCount == 0) {
        SerialMon.println("No I2C devices found.");
    }
    SerialMon.println();
}

void scanConfiguredI2cPins(const char *powerLabel)
{
    Wire.end();
    delay(50);
    Wire.begin(BOARD_SDA_PIN, BOARD_SCL_PIN);
    Wire.setClock(100000);

    SerialMon.print("I2C SDA=");
    SerialMon.print(BOARD_SDA_PIN);
    SerialMon.print(", SCL=");
    SerialMon.println(BOARD_SCL_PIN);
    scanI2cBus(powerLabel);
}

void setSensorPowerLevel(uint8_t level)
{
    pinMode(BOARD_SENSOR_POWER_EN_PIN, OUTPUT);
    digitalWrite(BOARD_SENSOR_POWER_EN_PIN, level);
    SerialMon.print("Sensor power GPIO ");
    SerialMon.print(BOARD_SENSOR_POWER_EN_PIN);
    SerialMon.println(level == HIGH ? " HIGH." : " LOW.");
    delay(500);
}

void setup()
{
    SerialMon.begin(SERIAL_BAUD);
    delay(500);

    SerialMon.println();
    SerialMon.println("ADS1115 I2C power polarity test starting.");
    enableBoardPowerRail();

    setSensorPowerLevel(HIGH);
    scanConfiguredI2cPins("sensor power HIGH");

    setSensorPowerLevel(LOW);
    scanConfiguredI2cPins("sensor power LOW");

    SerialMon.println("Expected ADS1115 address is 0x48 because ADDR is tied to GND.");
    SerialMon.println("Repeating every 5 seconds.");
}

void loop()
{
    setSensorPowerLevel(HIGH);
    scanConfiguredI2cPins("sensor power HIGH");

    setSensorPowerLevel(LOW);
    scanConfiguredI2cPins("sensor power LOW");

    delay(5000);
}
