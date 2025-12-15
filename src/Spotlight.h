#pragma once

#include <Arduino.h>

#include "Config.h"

namespace Spotlight {
inline void init() {
  for (uint8_t i = 0; i < Config::Spotlight::SPOTLIGHT_COUNT; i++) {
    pinMode(Config::Spotlight::SPOTLIGHT_PINS[i], OUTPUT);
    digitalWrite(Config::Spotlight::SPOTLIGHT_PINS[i], LOW);
  }
}
inline void on(uint8_t index) {
  if (index >= Config::Spotlight::SPOTLIGHT_COUNT) {
    return;
  }
  digitalWrite(Config::Spotlight::SPOTLIGHT_PINS[index], HIGH);
  return;
}
inline void off(uint8_t index) {
  if (index >= Config::Spotlight::SPOTLIGHT_COUNT) {
    return;
  }
  digitalWrite(Config::Spotlight::SPOTLIGHT_PINS[index], LOW);
}
inline void allOff() {
  for (uint8_t i = 0; i < Config::Spotlight::SPOTLIGHT_COUNT; i++) {
    digitalWrite(Config::Spotlight::SPOTLIGHT_PINS[i], LOW);
  }
}
} // namespace Spotlight
