#include <Arduino.h>

#define FAN0_PIN PA8

void setup() {
  pinMode(FAN0_PIN, OUTPUT);
}

void loop() {
  digitalWrite(FAN0_PIN, HIGH);
  delay(3000);
  digitalWrite(FAN0_PIN, LOW);
  delay(3000);
}
