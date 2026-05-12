#include <Arduino.h>

const int baudrate = 115200;
const int ledPin = 13;
const int ldrPin = 36;

void setup() {
  Serial.begin(baudrate);
  // Serial.println("Hello World!");
  Serial.println("Write something: ");
  pinMode(ledPin, OUTPUT);
  pinMode(ldrPin, INPUT);
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readString();
    Serial.println("You wrote: " + input);
  }

  // digitalWrite(ledPin, HIGH);
  // delay(1000);
  // digitalWrite(ledPin, LOW);
  delay(100);

  int value = analogRead(ldrPin);
  Serial.println(value);

}