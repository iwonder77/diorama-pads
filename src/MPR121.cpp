#include "MPR121.h"
#include "Config.h"

MPR121::MPR121() {}

bool MPR121::begin(uint8_t i2caddr, TwoWire *theWire, uint8_t touchThreshold,
                   uint8_t releaseThreshold, bool autoconfig) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(i2caddr, theWire);
  delay(10);

  if (!i2c_dev->begin()) {
    Serial.print(i2caddr, HEX);
    Serial.println(": I2C Fail");
    return false;
  }

  // reset all registers with a soft reset
  writeRegister(MPR121_SOFTRESET, 0x63);

  Serial.println("Initial CDC and CDT Values:");
  dumpCDCandCDTRegisters();
  delay(10);

  // ------------ CRITICAL: Enter STOP mode before configuration ------------
  // DATASHEET specifically states in sec 5.1 that "register write operation can
  // only be done in Stop Mode"
  writeRegister(MPR121_ECR, 0x00);
  delay(10);
  // -------------------------------------------------------------------------

  uint8_t c = readRegister8(MPR121_CONFIG2);
  if (c != 0x24) {
    Serial.print(i2caddr, HEX);
    Serial.println(": Reset Fail");
    return false;
  }

  setThresholds(touchThreshold, releaseThreshold);

  // ---------- CONFIG1 & CONFIG2 REGISTERS ----------
  writeRegister(MPR121_CONFIG1, Config::Touch::REG_CONFIG1);
  writeRegister(MPR121_CONFIG2, Config::Touch::REG_CONFIG2);
  // ----------------------------------------

  // ---------- BASELINE TRACKING REGISTERS ----------
  writeRegister(MPR121_MHDF, Config::Touch::MHDF);
  writeRegister(MPR121_NHDF, Config::Touch::NHDF);
  writeRegister(MPR121_NCLF, Config::Touch::NCLF);
  writeRegister(MPR121_FDLF, Config::Touch::FDLF);

  writeRegister(MPR121_MHDR, Config::Touch::MHDR);
  writeRegister(MPR121_NHDR, Config::Touch::NHDR);
  writeRegister(MPR121_NCLR, Config::Touch::NCLR);
  writeRegister(MPR121_FDLR, Config::Touch::FDLR);

  writeRegister(MPR121_NHDT, 0x00);
  writeRegister(MPR121_NCLT, 0x00);
  writeRegister(MPR121_FDLT, 0x00);
  // ------------------------------------------------------

  // ---------- DEBOUNCE ----------
  writeRegister(MPR121_DEBOUNCE, 0);
  // ------------------------------

  // ---------- AUTOCONFIG ----------
  setAutoconfig(autoconfig);
  // --------------------------------

  // ---------- TRANSITION TO RUN Mode ----------
  writeRegister(MPR121_ECR, Config::Touch::REG_ECR_RUN);
  // --------------------------------------------

  // auto-config triggers after transition to run mode (previous writeRegister()
  // line) if the `autoconfig` argument is true
  // give it some time to complete before outputting CDCx/CDTx values
  delay(1000);

  Serial.println("New CDC and CDT Values:");
  dumpCDCandCDTRegisters();

  return true;
}

uint16_t MPR121::filteredData(uint8_t electrode) {
  if (electrode >= 12)
    return 0;
  uint8_t lsb =
      readRegister8(MPR121_FILTDATA_0L + 2 * electrode); // will return 8 bits
  uint8_t msb =
      readRegister8(MPR121_FILTDATA_0H + 2 * electrode); // will return 2 bits
  uint16_t filtered =
      ((uint16_t)(msb & 0x03) << 8) | lsb; // combined into 10-bit value
  return filtered;
}

uint16_t MPR121::baselineData(uint8_t electrode) {
  if (electrode >= 12)
    return 0;
  // read 8-bit baseline REGISTERS (these 8-bits actually represent the upper 8
  // bits of a 10-bit system)
  uint8_t baseline_raw = readRegister8(MPR121_BASELINE_0 + electrode);
  uint16_t baseline = baseline_raw
                      << 2; // left shift to match filtered data scale (10-bits)
  return (baseline);
}

uint8_t MPR121::readRegister8(uint8_t reg) {
  Adafruit_BusIO_Register thereg = Adafruit_BusIO_Register(i2c_dev, reg, 1);

  return (thereg.read());
}

void MPR121::writeRegister(uint8_t reg, uint8_t value) {
  // MPR121 must be put in Stop Mode to write to most registers
  bool stop_required = true;

  // first get the current set value of the MPR121_ECR register
  Adafruit_BusIO_Register ecr_reg =
      Adafruit_BusIO_Register(i2c_dev, MPR121_ECR, 1);

  uint8_t ecr_backup = ecr_reg.read();
  if ((reg == MPR121_ECR) || ((0x73 <= reg) && (reg <= 0x7A))) {
    stop_required = false;
  }

  if (stop_required) {
    // clear this register to set stop mode
    ecr_reg.write(0x00);
  }

  Adafruit_BusIO_Register the_reg = Adafruit_BusIO_Register(i2c_dev, reg, 1);
  the_reg.write(value);

  if (stop_required) {
    // write back the previous set ECR settings
    ecr_reg.write(ecr_backup);
  }
}

void MPR121::setThresholds(uint8_t touch, uint8_t release) {
  // set all thresholds (the same)
  for (uint8_t i = 0; i < 12; i++) {
    writeRegister(MPR121_TOUCHTH_0 + 2 * i, touch);
    writeRegister(MPR121_RELEASETH_0 + 2 * i, release);
  }
}

void MPR121::setAutoconfig(boolean autoconfig) {
  if (autoconfig) {
    // enable auto-reconfiguration and auto-configuration bits of
    // AUTOCONFIG0 register and write to them
    uint8_t are = 1;
    uint8_t ace = 1;
    uint8_t reg_autoconfig0_true = (Config::Touch::FFI << 6) |
                                   (Config::Touch::RETRY << 4) |
                                   (Config::Touch::BVA << 2) | (are << 1) | ace;

    writeRegister(MPR121_AUTOCONFIG0, reg_autoconfig0_true);

    writeRegister(MPR121_UPLIMIT, Config::Touch::USL);
    writeRegister(MPR121_TARGETLIMIT, Config::Touch::TL);
    writeRegister(MPR121_LOWLIMIT, Config::Touch::LSL);
    // note the autoconfiguration will take effect when we transition from
    // STOP -> RUN modes
  } else {
    writeRegister(MPR121_AUTOCONFIG0, Config::Touch::REG_AUTOCONFIG0);
  }
}

uint16_t MPR121::touched() {
  uint16_t touchMask = 0;

  for (uint8_t i = 0; i < Config::Touch::NUM_ELECTRODES; i++) {
    uint16_t f = filteredData(i);
    uint16_t b = baselineData(i);
    int16_t d = (int16_t)f - (int16_t)b;

    // smoothen out delta readings with ema filter
    smoothDelta[i] += Config::Touch::ALPHA * (d - smoothDelta[i]);
    if (smoothDelta[i] > 0) {
      Serial.print(" ");
    }
    Serial.print(smoothDelta[i]);
    Serial.print(", ");

    // --- TOUCH DETECTION (w/ hysteresis + debounce) ---
    if (!isTouched[i]) {
      // candidate for touch detection
      if (smoothDelta[i] < Config::Touch::DELTA_TOUCH_THRESHOLD) {
        if (++touchDebounceCount[i] >= Config::Touch::DEBOUNCE_COUNT) {
          isTouched[i] = true;         // mark electrode as touched
          releaseDebounceCount[i] = 0; // start release counter at 0
        }
      } else {
        touchDebounceCount[i] = 0; // reset touch counter
      }
    }
    // --- RELEASE DETECTION (also w/ hysteresis + debounce) ---
    else {
      // candidate for release detection (currently touched)
      if (smoothDelta[i] > Config::Touch::DELTA_RELEASE_THRESHOLD) {
        if (++releaseDebounceCount[i] >= Config::Touch::DEBOUNCE_COUNT) {
          isTouched[i] = false;      // mark as untouched
          touchDebounceCount[i] = 0; // start touch counter at 0
        }
      } else {
        releaseDebounceCount[i] = 0; // reset release counter
      }
    }

    // build output bitmask
    if (isTouched[i]) {
      touchMask |= (1 << i);
    }
  }
  Serial.println();

  return touchMask;
}

void MPR121::verifyRegisters() {
  Serial.println("\n======= REGISTER VERIFICATION =======");

  // Check if we're in RUN or STOP mode
  uint8_t ecr = readRegister8(MPR121_ECR);
  Serial.print("ECR (0x5E): 0x");
  Serial.print(ecr, HEX);
  Serial.println(ecr == 0x00 ? " [STOP MODE]" : " [RUN MODE]");

  // CONFIG1
  uint8_t cfg1 = readRegister8(MPR121_CONFIG1);
  Serial.print("CONFIG1 (0x5C): 0x");
  Serial.print(cfg1, HEX);
  Serial.print("  FFI=");
  Serial.print((cfg1 >> 6) & 0x03);
  Serial.print(", CDC_global=");
  Serial.println(cfg1 & 0x3F);

  // CONFIG2
  uint8_t cfg2 = readRegister8(MPR121_CONFIG2);
  Serial.print("CONFIG2 (0x5D): 0x");
  Serial.print(cfg2, HEX);
  Serial.print("  CDT_global=");
  Serial.print((cfg2 >> 5) & 0x07);
  Serial.print(", SFI=");
  Serial.print((cfg2 >> 3) & 0x03);
  Serial.print(", ESI=");
  Serial.println(cfg2 & 0x07);

  // AUTOCONFIG0
  uint8_t ac0 = readRegister8(MPR121_AUTOCONFIG0);
  Serial.print("AUTOCONFIG0 (0x7B): 0x");
  Serial.print(ac0, HEX);
  Serial.print("  ACE=");
  Serial.print(ac0 & 0x01);
  Serial.print(", ARE=");
  Serial.println((ac0 >> 1) & 0x01);

  // Baseline tracking registers
  Serial.print("MHDF (0x2F): ");
  Serial.println(readRegister8(0x2F), HEX);
  Serial.print("NHDF (0x30): ");
  Serial.println(readRegister8(0x30), HEX);
  Serial.print("NCLF (0x31): ");
  Serial.println(readRegister8(0x31), HEX);
  Serial.print("FDLF (0x32): ");
  Serial.println(readRegister8(0x32), HEX);

  // Individual CDC values
  Serial.print("Individual CDC [E0-E2]: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(readRegister8(0x5F + i));
    Serial.print(" ");
  }
  Serial.println();

  // Individual CDT values
  Serial.print("Individual CDT [E0-E2]: ");
  uint8_t cdt01 = readRegister8(0x6C);
  uint8_t cdt23 = readRegister8(0x6D);
  Serial.print(cdt01 & 0x07); // E0
  Serial.print(" ");
  Serial.print((cdt01 >> 4) & 0x07); // E1
  Serial.print(" ");
  Serial.print(cdt23 & 0x07); // E2
  Serial.println();

  Serial.println("======================================\n");
}

void MPR121::dumpCapData(uint8_t electrode) {
  if (electrode > 11)
    return;

  uint16_t filtered = filteredData(electrode);
  uint16_t baseline = baselineData(electrode);
  int16_t delta = (int16_t)filtered - (int16_t)baseline;

  // print results
  Serial.print(electrode);
  Serial.print(",");
  Serial.print(filtered);
  Serial.print(",");
  Serial.print(baseline);
  Serial.print(",");
  Serial.print(delta);
  Serial.println();
}

void MPR121::dumpCDCandCDTRegisters() {
  Serial.println("ELECTRODE: 00 01 02 03 04 05 06 07 08 09 10 11");
  Serial.println("           -- -- -- -- -- -- -- -- -- -- -- --");

  // CDC - Charge Discharge Current
  Serial.print("CDC:       ");
  for (int i = 0; i < 12; i++) {
    uint8_t reg = readRegister8(0x5F + i);
    if (reg < 10)
      Serial.print(" ");
    Serial.print(reg);
    Serial.print(" ");
  }
  Serial.println();

  // CDT - Charge Discharge Time
  Serial.print("CDT:       ");
  for (int i = 0; i < 6; i++) {
    // read CDT registers (6 total)
    uint8_t reg = readRegister8(0x6C + i);
    uint8_t cdt_even = reg & 0b111; // even numbered electrode CDT -> bits[2:0]
    uint8_t cdt_odd =
        (reg >> 4) & 0b111; // odd numbered electrode CDT -> bits[6:4]
    if (cdt_even < 10)
      Serial.print(" ");
    Serial.print(cdt_even);
    Serial.print(" ");
    if (cdt_odd < 10)
      Serial.print(" ");
    Serial.print(cdt_odd);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println();
}
