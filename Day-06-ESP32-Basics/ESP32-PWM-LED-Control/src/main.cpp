#include <Arduino.h>

const int LED_PIN = 4;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

int brightness = 0;

void setup() {
    Serial.begin(115200);

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED_PIN, PWM_CHANNEL);

    ledcWrite(PWM_CHANNEL, 0);

    Serial.println("Commands:");
    Serial.println("ON");
    Serial.println("OFF");
    Serial.println("BRIGHTNESS <0-255>");
}

void loop() {
    if (Serial.available()) {

        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.equalsIgnoreCase("ON")) {

            brightness = 255;
            ledcWrite(PWM_CHANNEL, brightness);
            Serial.println("LED ON");

        }
        else if (command.equalsIgnoreCase("OFF")) {

            brightness = 0;
            ledcWrite(PWM_CHANNEL, brightness);
            Serial.println("LED OFF");

        }
        else if (command.startsWith("BRIGHTNESS")) {

            int value = command.substring(10).toInt();

            if (value >= 0 && value <= 255) {

                brightness = value;
                ledcWrite(PWM_CHANNEL, brightness);

                Serial.print("Brightness: ");
                Serial.println(brightness);

            } else {

                Serial.println("Brightness must be between 0 and 255");

            }
        }
        else {

            Serial.println("Invalid command");

        }
    }
}