#ifndef APP_H
#define APP_H

#include <Arduino.h>

#include "Config.h"
#include "MPR121.h"
#include "NeoPixelRing.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

class App {
public:
  App(Config::AppState appState = Config::AppState::DEBUG)
      : state_(appState), neop_ring_(Config::LED_DATA_PIN, Config::LED_COUNT,
                                     Config::BRIGHTNESS, Config::WARM_WHITE) {}
  bool setup();
  void loopOnce();

private:
  void runDebug();
  void run();

  uint16_t last_touched_ = 0;
  uint16_t curr_touched_ = 0;

  // timestamps to track when spotlights first turn on
  uint32_t spotlights_on_at_[Config::Spotlight::SPOTLIGHT_COUNT] = {0};

  // boolean to track if a spotlight is on
  bool spotlight_on_[Config::Spotlight::SPOTLIGHT_COUNT] = {false};

  Config::AppState state_;
  MPR121 mpr121_;
  NeoPixelRing neop_ring_;
};

#endif
