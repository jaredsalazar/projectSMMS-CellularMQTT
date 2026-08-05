const uint8_t SENSOR_POWER_PIN = 19;
const uint32_t HOLD_TIME_MS = 10000UL;

void setup()
{
    Serial.begin(115200);
    delay(500);

    pinMode(SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, LOW);

    Serial.println();
    Serial.println("GPIO 19 power test starting");
}

void loop()
{
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    Serial.println("Pin 19 HIGH for 10 seconds");
    delay(HOLD_TIME_MS);

    digitalWrite(SENSOR_POWER_PIN, LOW);
    Serial.println("Pin 19 LOW for 10 seconds");
    delay(HOLD_TIME_MS);
}
