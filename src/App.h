#ifndef APP_H
#define APP_H

#include <Arduino.h>

#include "AudioPlayer.h"
#include "MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

enum class AppState { DEBUG, RUN_PROD, ERROR_RECOVERY };
enum class ProductionRunState { IDLE, PLAYING };

class App {
public:
  bool setup();
  void loopOnce();

private:
  void runDebug();
  void runProd();

  // bool setAutoconfig = false;
  uint16_t lastTouched = 0;
  uint16_t currTouched = 0;

  AppState state = AppState::DEBUG;
  ProductionRunState currentProdRunState = ProductionRunState::IDLE;
  MPR121 mpr121;
  AudioPlayer player;
};

#endif
