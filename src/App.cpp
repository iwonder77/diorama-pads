#include "App.h"
#include "Config.h"
#include "Spotlight.h"

bool App::setup() {
  pinMode(Config::Audio::AUDIO_BUSY, INPUT);

  Spotlight::init();
  Spotlight::allOff();

  if (!mpr121.begin()) {
    Serial.print("MPR121 not found, check wiring");
    return false;
  }

  delay(1000);

  mpr121.verifyRegisters();

  if (state == Config::AppState::DEBUG) {
    Serial.println("=== FULL ELECTRODE READINGS ===");
    Serial.println("Electrode, Filtered, Baseline, Delta");
  }
  return true;
}

void App::loopOnce() {
  switch (state) {
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
    mpr121.dumpCapData(i);
  }
  Serial.println();
  delay(10);
}

// NOTE: query MPR121's touched() method only when system is in IDLE or in
// COOLDOWN (not playing sounds) to avoid speaker interference messing with
// baseline tracking and delta updates
void App::run() {
  bool isPlaying = (digitalRead(Config::Audio::AUDIO_BUSY) == LOW);

  switch (currentRunState) {
  case RunState::IDLE:
    currTouched = mpr121.touched();
    for (uint8_t i = 0; i < Config::Touch::NUM_ELECTRODES; i++) {
      if ((currTouched & _BV(i)) && !(lastTouched & _BV(i))) {
        // if it *is* touched and *wasnt* touched before, send UART command to
        // DY-HV20T to play track, wait for busy pin to assert, then
        // transition state
        player.playTrackByIndex((uint16_t)i + 1);

        playbackBeganAt = millis();
        currentRunState = RunState::PLAYING;

        Spotlight::on(i);
        break;
      }
    }
    lastTouched = currTouched;
    break;
  case RunState::PLAYING:
    // wait for busy pin to assert
    if (millis() - playbackBeganAt < Config::Audio::AUDIO_BEGIN_TIMEOUT_MS)
      break;
    // safety timeout: if playback exceeds maximum expected duration, force exit
    if (millis() - playbackBeganAt > Config::Audio::AUDIO_MAX_DURATION_MS) {
      Serial.println("WARN: Playback timeout, forcing IDLE");
      currentRunState = RunState::COOLDOWN;
      playbackEndedAt = millis();
      break;
    }
    if (!isPlaying) {
      // only transition back to COOLDOWN state when sound is done playing
      currentRunState = RunState::COOLDOWN;

      Serial.println("DONE PLAYING");

      // create a timestamp when playback ended to begin cooldown period
      playbackEndedAt = millis();
    } else {
      Serial.println("PLAYING");
    }
    break;
  case RunState::COOLDOWN:
    // allow MPR121 to take readings/update delta during cooldown period but
    // don't do anything with them, this is to ensure stale readings from last
    // transition and ema filter are updated
    Serial.println("cooling down!");
    currTouched = mpr121.touched();
    lastTouched = currTouched;
    if (millis() - playbackEndedAt >= Config::Audio::AUDIO_END_COOLDOWN_MS) {
      Spotlight::allOff();
      currentRunState = RunState::IDLE;
    }
    break;
  }

  // calm lil delay to prevent overwhelming mcu
  delay(10);
}
