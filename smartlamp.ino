#include <Arduino.h>

int ledPin = 13;
int ledValue = 100;

int ldrPin = 36;
int ldrMax = 4000;

void setup() {
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    
    Serial.println("SmartLamp Initialized.");
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        processCommand(command);
    }
}

void processCommand(String command) {
    if (command.startsWith("SET_LED ")) {
        int value = command.substring(8).toInt();
        Serial.println("ok");
        if (value >= 0 && value <= 100) {
            ledUpdate(value);
            Serial.println("RES SET_LED 1");
        } else {
            Serial.println("RES SET_LED -1");
        }
    } else if (command == "GET_LED") {
        Serial.printf("RES GET_LED %d\n", ledValue);
    } else if (command == "GET_LDR") {
        int ldrValue = ldrGetValue();
        Serial.printf("RES GET_LDR %d\n", ldrValue);
    } else {
        Serial.println("ERR Unknown command.");
    }
}

void ledUpdate(int newValue) {
    ledValue = newValue;
    int pwmValue = map(ledValue, 0, 100, 0, 255);
    analogWrite(ledPin, pwmValue);
}

int ldrGetValue() {
    int digitalValue = digitalRead(ldrPin);
    return digitalValue == HIGH ? 100 : 0;
}
