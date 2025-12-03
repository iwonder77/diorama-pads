#ifndef APP_H
#define APP_H

#include <Arduino.h>

#include "AudioPlayer.h"
#include "MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

enum class ProductionRunState { IDLE, PLAYING };

class App {
public:
  App(Config::AppState appState = Config::AppState::DEBUG) : state(appState) {}
  bool setup();
  void loopOnce();

private:
  void runDebug();
  void runProd();

  // bool setAutoconfig = false;
  uint16_t lastTouched = 0;
  uint16_t currTouched = 0;

  Config::AppState state;
  ProductionRunState currentProdRunState = ProductionRunState::IDLE;
  MPR121 mpr121;
  AudioPlayer player;
};

#endif
