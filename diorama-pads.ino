#include <Wire.h>
#include "src/App.h"
#include "src/Config.h"

App app;

void setup() {
  Serial.begin(9600);
  delay(100);
  Wire.begin();
  delay(100);
  Serial1.begin(9600, SERIAL_8N1, Config::Audio::AUDIO_RXI, Config::Audio::AUDIO_TXO);
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
