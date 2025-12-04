#ifndef MPR121_H
#define MPR121_H

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Arduino.h>

#include "Config.h"

// =============================================
// MPR121 Register Map (see datasheet for more)
// =============================================
enum {
  // --- Electrode Touch Status Registers ---
  MPR121_TOUCHSTATUS_L = 0x00,
  MPR121_TOUCHSTATUS_H = 0x01,

  // --- Initial Filtered Data Registers ---
  MPR121_FILTDATA_0L = 0x04, // 8 least-siginificant bits of electrode 0
  MPR121_FILTDATA_0H = 0x05, // 2 most-significant bits of electrode 0

  // --- Initial Baseline Data Register ---
  MPR121_BASELINE_0 = 0x1E,

  // --- Baseline Tracking Registers ---
  // RISING (filtered > baseline)
  MPR121_MHDR = 0x2B,
  MPR121_NHDR = 0x2C,
  MPR121_NCLR = 0x2D,
  MPR121_FDLR = 0x2E,
  // FALLING (filtered < baseline)
  MPR121_MHDF = 0x2F,
  MPR121_NHDF = 0x30,
  MPR121_NCLF = 0x31,
  MPR121_FDLF = 0x32,
  // TOUCHED
  MPR121_NHDT = 0x33,
  MPR121_NCLT = 0x34,
  MPR121_FDLT = 0x35,

  // --- Initial Touch/Release Threshold Registers ---
  MPR121_TOUCHTH_0 = 0x41,
  MPR121_RELEASETH_0 = 0x42,

  // --- Debounce Register ---
  MPR121_DEBOUNCE = 0x5B,

  // --- Filter + Global CDC/CDT Configuration Registers ---
  MPR121_CONFIG1 = 0x5C, // FFI + global CDC
  MPR121_CONFIG2 = 0x5D, // CDT + SFI + ESI
  MPR121_ECR =
      0x5E, // electrode configuration register (for STOP/RUN Mode activation)

  // --- Individual Electrode CDC (one register per electrode) ---
  MPR121_CHARGECURR_0 = 0x5F, // through 0x6A for electrodes 0-11

  // --- Individual Electrode CDT (packed, 2 electrodes per register) ---
  MPR121_CHARGETIME_1 = 0x6C, // throuhg 0x71 for electrodes 0-11

  // --- Auto-configuration Registers ---
  MPR121_AUTOCONFIG0 = 0x7B,
  MPR121_AUTOCONFIG1 = 0x7C,
  MPR121_UPLIMIT = 0x7D,
  MPR121_LOWLIMIT = 0x7E,
  MPR121_TARGETLIMIT = 0x7F,

  // --- Soft reset register ---
  MPR121_SOFTRESET = 0x80,
};

class MPR121 {
public:
  MPR121();
  bool begin(uint8_t i2caddr = Config::Touch::MPR121_I2C_ADDR,
             TwoWire *theWire = &Wire,
             uint8_t touchThreshold = Config::Touch::TOUCH_THRESHOLD,
             uint8_t releaseThreshold = Config::Touch::RELEASE_THRESHOLD,
             bool autoconfig = true);

  uint16_t filteredData(uint8_t electrode);
  uint16_t baselineData(uint8_t electrode);

  uint8_t readRegister8(uint8_t reg);
  void writeRegister(uint8_t reg, uint8_t value);
  void setThresholds(uint8_t touch, uint8_t release);
  void setAutoconfig(bool autoconfig);

  uint16_t touched();

  void verifyRegisters();
  void dumpCapData(uint8_t electrode);
  void dumpCDCandCDTRegisters();

private:
  Adafruit_I2CDevice *i2c_dev = NULL;

  float smoothDelta[Config::Touch::NUM_ELECTRODES] = {0.0f};
  uint8_t touchDebounceCount[Config::Touch::NUM_ELECTRODES] = {0};
  uint8_t releaseDebounceCount[Config::Touch::NUM_ELECTRODES] = {0};
  bool isTouched[Config::Touch::NUM_ELECTRODES] = {false};
};

#endif
