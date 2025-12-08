#include <esp_task_wdt.h>
#include <Wire.h>

#include "src/App.h"
#include "src/Config.h"

App app = App(Config::AppState::RUN);

// Task Watchdog Timer Configuration Struct
esp_task_wdt_config_t twdt_config = {
  .timeout_ms = Config::WATCHDOG_TIMEOUT_MS,
  .idle_core_mask = (1 << 0),  // ESP32-C3 has only one core
  .trigger_panic = true
};

void setup() {
  Serial.begin(9600);
  delay(100);
  Wire.begin();
  delay(100);
  Serial1.begin(9600, SERIAL_8N1, Config::Audio::AUDIO_RXI, Config::Audio::AUDIO_TXO);
  delay(3000);

  // initialize the Task Watchdog
  // (TWDT is a hardware-backed timer tied to a specific FreeRTOS task)
  esp_task_wdt_init(&twdt_config);
  // add/register task for watchdog to monitor
  // in Arduino ESP32 environment, our `loop()` runs inside the FreeRTOS task named `loopTask`
  // register it here with the following
  esp_task_wdt_add(NULL);

  uint8_t attempts = 0;

  while (attempts < Config::MAX_RETRY_ATTEMPTS) {
    if (app.setup()) {
      break;  // success
    }

    Serial.println("App setup failed; retrying...");
    attempts++;

    // feed watchdog during retry events
    esp_task_wdt_reset();
    delay(100);
  }

  if (attempts == Config::MAX_RETRY_ATTEMPTS) {
    Serial.println("App setup failed, check wiring...");
    Serial.println("Letting watchdog reset.");
    while (1);  // intentionally enter infinite loop to let watchdog reset firmware
  }
}

void loop() {
  esp_task_wdt_reset();
  app.loopOnce();
}
