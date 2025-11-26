#ifndef MY_MPR121_H
#define MY_MPR121_H
#define I2CADDR_DEFAULT 0x5A

#include "Arduino.h"
#include <Adafruit_MPR121.h>

class MyMPR121 : public Adafruit_MPR121 {
public:
  MyMPR121();
  bool begin(uint8_t i2c_addr = I2CADDR_DEFAULT, TwoWire *the_wire = &Wire);
  void dumpCapData(uint8_t electrode);
  void dumpCDCandCDTRegisters();
};

#endif
