#include <Arduino.h>

#define LED_PIN 2
#define SWITCH_PIN 4

void setup()
{
    pinMode(LED_PIN, OUTPUT);

    // Internal pull-up resistor enabled
    pinMode(SWITCH_PIN, INPUT_PULLUP);

    Serial.begin(115200);
}

void loop()
{
    if (digitalRead(SWITCH_PIN) == LOW)
    {
        // Switch pressed
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Switch Pressed - LED ON");
    }
    else
    {
        // Switch released
        digitalWrite(LED_PIN, LOW);
        Serial.println("Switch Released - LED OFF");
    }

    delay(100);
}