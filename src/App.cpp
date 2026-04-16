#include "App.h"
#include "Spotlight.h"

bool App::setup() {
  Spotlight::init();
  Spotlight::allOff();

  if (!mpr121_.begin()) {
    Serial.print("MPR121 not found, check wiring");
    return false;
  }

  delay(1000);

  mpr121_.verifyRegisters();

  if (state_ == Config::AppState::DEBUG) {
    Serial.println("=== FULL ELECTRODE READINGS ===");
    Serial.println("Electrode, Filtered, Baseline, Delta");
  }
  return true;
}

void App::loopOnce() {
  switch (state_) {
  case Config::AppState::DEBUG:
    runDebug();
    break;
  case Config::AppState::RUN:
    run();
    break;
  case Config::AppState::ERROR_RECOVERY:
    // TODO: implement recover() method
    break;
  default:
    break;
  }
}

void App::runDebug() {
  for (uint8_t i = 0; i < Config::Touch::NUM_ELECTRODES; i++) {
    mpr121_.dumpCapData(i);
  }
  Serial.println();
  delay(10);
}

// NOTE: query MPR121's touched() method only when system is in IDLE or in
// COOLDOWN (not playing sounds) to avoid speaker interference messing with
// baseline tracking and delta updates
void App::run() {
  // always take readings to turn on multiple spotlights at once
  curr_touched_ = mpr121_.touched();
  for (uint8_t i = 0; i < Config::Touch::NUM_ELECTRODES; i++) {
    if ((curr_touched_ & _BV(i)) && !(last_touched_ & _BV(i))) {
      // if it *is* touched and *wasnt* touched before, turn on spotlight and
      // take timestamp for brief cooldown period
      Spotlight::on(i);
      spotlights_on_at_[i] = millis();
      spotlight_on_[i] = true;
    }
    if (spotlight_on_[i] && millis() - spotlights_on_at_[i] >
                                Config::Spotlight::SPOTLIGHT_ON_PERIOD_MS) {
      // if spotlight is on and its on period has passed, turn off and restart
      // timestamp
      Spotlight::off(i);
      spotlight_on_[i] = false;
      spotlights_on_at_[i] = 0;
    }
  }
  last_touched_ = curr_touched_;

  // calm lil delay to prevent overwhelming mcu
  delay(10);
}
