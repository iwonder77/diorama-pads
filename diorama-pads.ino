#include <Wire.h>
#include "src/App.h"

#define RXI_PIN 20  // ESP32C3 RXI -> DY-HV20T TX
#define TXO_PIN 21  // ESP32C3 TXO -> DY-HV20T RX

App app;

void setup() {
  Serial.begin(9600);
  delay(100);
  Wire.begin();
  delay(100);
  Serial1.begin(9600, SERIAL_8N1, RXI_PIN, TXO_PIN);
  delay(3000);

  if (!app.setup()) {
    // failed, notify then hold indefinitely for now
    Serial.println("SETUP FAILED");
    delay(1000);
    while (1);
  }
}

void loop() {
  app.loopOnce();
}
