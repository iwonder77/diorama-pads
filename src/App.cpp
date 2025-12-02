#include "App.h"
#include <Wire.h>

bool App::setup() {
  pinMode(BUSY_PIN, INPUT);

  if (!mpr121.begin(MPR121_I2C_ADDR, &Wire)) {
    Serial.print("MPR121 not found, check wiring");
    return false;
  }

  delay(1000);

  mpr121.verifyRegisters();

  if (setAutoconfig) {
    Serial.println("Initial CDC and CDT Values:");
    mpr121.dumpCDCandCDTRegisters();
    delay(1000);

    // mpr121.setAutoconfig(setAutoconfig);

    Serial.println("New CDC and CDT Values:");
    mpr121.dumpCDCandCDTRegisters();
    delay(1000);
  }

  if (state == AppState::DEBUG) {
    Serial.println("Global CDC and CDT Values will be used");
    mpr121.dumpCDCandCDTRegisters();
    delay(1000);
    Serial.println("=== ELECTRODE READINGS ===");
    Serial.println("Electrode, Filtered, Baseline, Delta");
  }
  return true;
}

void App::loopOnce() {
  switch (state) {
  case AppState::DEBUG:
    runDebug();
    break;
  case AppState::RUN_PROD:
    runProd();
    break;
  case AppState::ERROR_RECOVERY:
    // TODO: implementrecover() method
    break;
  default:
    break;
  }
}

void App::runDebug() {
  for (uint8_t i = 0; i < NUM_ELECTRODES; i++) {
    mpr121.dumpCapData(i);
  }
  Serial.println();
  delay(10);
}

void App::runProd() {
  /*
  // read busy pin
  bool isPlaying = (digitalRead(BUSY_PIN) == LOW);

  // touched method returns a 16-bit value where each bit represents one pad
  // so if pads 0 and 2 are currently touched:
  // currTouched = 0b0000_0000_0000_0101
  currTouched = mpr121.touched();

  switch (currentProdRunState) {
  case ProductionRunState::IDLE:
    for (uint8_t i = 0; i < NUM_ELECTRODES; i++) {
      // it if *is* touched and *wasnt* touched before, begin playing and
      // immediately transition state!
      if ((currTouched & _BV(i)) && !(lastTouched & _BV(i))) {
        Serial.print(i);
        Serial.println(" touched");
        player.playTrackByIndex((uint16_t)i + 1);
        currentProdRunState = ProductionRunState::PLAYING;
        break;
      }
    }
    break;
  case ProductionRunState::PLAYING:
    if (!isPlaying) {
      // only transition back to IDLE state when sound is done playing
      currentProdRunState = ProductionRunState::IDLE;
    } else {
      Serial.println("PLAYING");
    }
    break;
  }
  lastTouched = currTouched;

  // put a delay so it isn't overwhelming
  delay(100);
  */
}
