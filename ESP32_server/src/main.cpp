#include <Arduino.h>

HardwareSerial core2Serial(2);

void setup() {
  Serial.begin(115200);
  delay(500);

  core2Serial.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("ESP32 bridge: PC -> CORE2 (hExt.serial)");
  Serial.println("Wysylaj znaki: A/a, B/b, C/c aby sterowac hLED1/2/3.");
}

void loop() {
  while (Serial.available() > 0) {
    int c = Serial.read();
    core2Serial.write(c);
  }

  while (core2Serial.available() > 0) {
    int c = core2Serial.read();
    Serial.write(c);
  }
}