#include <Arduino.h>

#include "AudioPlayer.h"
#include "MyMPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define BUSY_PIN A4

const uint8_t NUM_ELECTRODES = 3;
const uint8_t MPR121_I2C_ADDR = 0x5A;
enum class AppState { DEBUG, RUN_PROD, ERROR_RECOVERY };
enum class ProductionRunState { IDLE, PLAYING };

class App {
public:
  bool setup();
  void loopOnce();

private:
  void runDebug();
  void runProd();

  bool setAutoconfig = false;
  uint16_t lastTouched = 0;
  uint16_t currTouched = 0;

  AppState state = AppState::DEBUG;
  ProductionRunState currentProdRunState = ProductionRunState::IDLE;
  MyMPR121 mpr121 = MyMPR121();
  AudioPlayer player;
};
