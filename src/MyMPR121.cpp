#include "MyMPR121.h"

MyMPR121::MyMPR121() {}

bool MyMPR121::begin(uint8_t i2caddr, TwoWire *theWire, uint8_t touchThreshold,
                     uint8_t releaseThreshold, boolean autoconfig) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(i2caddr, theWire);
  delay(10);

  if (!i2c_dev->begin()) {
    Serial.print(i2caddr, HEX);
    Serial.println(": I2C Fail");
    return false;
  }

  // soft reset (reset all registers)
  writeRegister(MPR121_SOFTRESET, 0x63);

  // ============ CRITICAL: Enter STOP mode before configuration ============
  // DATASHEET specifically states in sec 5.1 that "register write operation can
  // only be done in Stop Mode"
  writeRegister(MPR121_ECR, 0x00);
  delay(10);
  // =========================================================================

  uint8_t c = readRegister8(MPR121_CONFIG2);
  if (c != 0x24) {
    Serial.print(i2caddr, HEX);
    Serial.println(": Reset Fail");
    return false;
  }

  setThresholds(touchThreshold, releaseThreshold);

  // ---------- CONFIG1 & CONFIG 2 ----------
  // (see pg 14 of datasheet for description and encoding values)
  // 1. Filter/Global CDC Configuration Register: 0x5C (CONFIG1)
  //    * FFI (First Filter Iterations) - bits [7:6]
  //      - sets samples taken by first level of filtering
  uint8_t ffi = 0b10; // 18 samples
  //    * CDC (global Charge/Discharge Current) - bits [5:0]
  //      - sets global supply current for each electrode
  uint8_t cdc_global = 0b100000; // sets current to 32μA
  uint8_t cfg1_reg = (ffi << 6) | (cdc_global & 0x3F);
  writeRegister(MPR121_CONFIG1, cfg1_reg);
  // 1. Filter/Global CDT Configuration Register: 0x5D (CONFIG2)
  //    * CDT (global Charge/Discharge Time) - bits [7:5]
  //      - sets global charge/discharge time for each electrode
  uint8_t cdt_global = 0b010; // sets charge time to 1μS
  //    * SFI (Second Filter Iterations) - bits [4:3]
  //      - sets number of samples for second level of filtering
  uint8_t sfi = 0b10; // 10 samples
  //    * ESI (Electrode Sample Interval) - bits [2:0]
  //      - electrode sampling period for second level filtering
  uint8_t esi = 0b001; // set to 2 ms
  uint8_t cfg2_reg = (cdt_global << 5) | (sfi << 3) | esi;
  writeRegister(MPR121_CONFIG2, cfg2_reg);
  // ----------------------------------------

  // ---------- SET BASELINE TRACKING PARAMETERS ----------
  // see page 12 (sec 5.5) of datasheet AND application
  // note AN3891 for baseline tracking and filtering info
  //
  // with the 0.2" PLA plate on the copper touch pads, the delta between
  // filtered data and baseline was quite small (around -30, BEFORE any changes
  // were made to signal strength, i.e. charge/discharge current and time) and
  // the MPR121's default baseline tracking system parameters for this FALLING
  // case led the baseline value to immediately track the filtered data
  // (believing it was noise/changes to environment instead of an actual touch
  // event) this rendered the pads useless in detecting touches
  //
  // we need to slow down this baseline tracking system by
  // setting new values for MHDF, NHDF, NCLF, and FDLF

  // ========== FALLING (filtered < baseline) ==========
  //  * MHDF = 4, small changes (±8 counts) are tracked immediately
  //    this is okay because tiny environmental changes should be followed
  uint8_t mhdf = 0x04;
  writeRegister(MPR121_MHDF, mhdf);
  //  * NHDF = 1, when baseline does adjust, move by small increment (1 count)
  uint8_t nhdf = 0x01;
  writeRegister(MPR121_NHDF, nhdf);
  //  * NCLF = 255, need 255 consecutive samples beyond small ±2 counts (MHDF)
  //    before adjusting baseline, at 100 samples/sec, every 2.55 seconds of
  //    sustained change will accumulate and change baseline (real drift), but
  //    brief touches won't
  uint8_t nclf = 0xFF;
  writeRegister(MPR121_NCLF, nclf);
  //  * FDLF = 6, add slight filter delay to slow the system even more
  uint8_t fdlf = 0x06;
  writeRegister(MPR121_FDLF, fdlf);

  // ========== RISING (filtered > baseline) ==========
  writeRegister(MPR121_MHDR, 0x01); // library default
  writeRegister(MPR121_NHDR, 0x01); // library default
  writeRegister(MPR121_NCLR, 0x04); // changed! was 0x0E = 14 before
  writeRegister(MPR121_FDLR, 0x00);

  // ========== TOUCH ==========
  writeRegister(MPR121_NHDT, 0x00);
  writeRegister(MPR121_NCLT, 0x00);
  writeRegister(MPR121_FDLT, 0x00);
  // ------------------------------------------------------

  // --- DEBOUNCE ---
  writeRegister(MPR121_DEBOUNCE, 0);
  // ----------------

  // ---------- AUTOCONFIG ----------
  // important: disable autoconfig so it doesn't overwrite our CDC/CDT values
  // Auto-Configuration Control Register 0: 0x7B (AUTOCONFIG0)
  //  * FFI (First Filter Iterations) - bits [7:6] (same as 0x5C)
  //  * RETRY - bits [5:4]
  //  * BVA - bits [3:2], fill with same bits as CL bits in ECR (0x5E) register
  //  * ARE (Auto-Reconfiguration Enable) - bit 1
  //  * ACE (Auto-Configuration Enable) - bit 0
  writeRegister(MPR121_AUTOCONFIG0, 0b10001000);
  // Auto-Configuration Control Register 1: 0x7C (AUTOCONFIG1)
  //  * SCTS (Skip Charge Time Search) - bit 7
  //  * bits [6:3] unused
  //  * OORIE (Out-of-range interrupt enable) - bit 2
  //  * ARFIE (Auto-reconfiguration fail interrupt enable) - bit 1
  //  * ACFIE (Auto-configuration fail interrupt enable) - bit 0
  // (see pg 17 of datasheet for description and encoding values)
  // --------------------------------

  // ---------- Re-enter RUN Mode ----------
  // Set ECR to enable all 12 electrodes
  //  * CL (Calibration Lock) - bits [7:6] = 0b10 (baseline tracking enabled
  //  with 5 bits)
  //  * ELEPROX_EN (Proximity Enable) - bits [5:4] = 0b00 (proximity detection
  //  disabled)
  //  * ELE_EN (Electrode Enable) - bits [3:0] = 0b1100 (run mode with electrode
  //  0 - 11 detection enabled)
  writeRegister(MPR121_ECR, 0x8C);
  // ---------------------------------------

  return true;
}

uint16_t MyMPR121::filteredData(uint8_t t) {
  if (t > 12)
    return 0;
  uint8_t lsb = readRegister8(0x04 + 2 * t); // will return 8 bits
  uint8_t msb = readRegister8(0x05 + 2 * t); // will return 2 bits
  uint16_t filtered =
      ((uint16_t)(msb & 0x03) << 8) | lsb; // combined into 10-bit value
  return filtered;
}

uint16_t MyMPR121::baselineData(uint8_t t) {
  if (t > 12)
    return 0;
  // read 8-bit baseline REGISTERS (these 8-bits acrually represent the upper 8
  // bits of a 10-bit system)
  uint8_t baseline_raw = readRegister8(0x1E + t);
  uint16_t baseline = baseline_raw
                      << 2; // left shift to match filtered data scale (10-bits)
  return (baseline);
}

uint8_t MyMPR121::readRegister8(uint8_t reg) {
  Adafruit_BusIO_Register thereg = Adafruit_BusIO_Register(i2c_dev, reg, 1);

  return (thereg.read());
}

void MyMPR121::writeRegister(uint8_t reg, uint8_t value) {
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

void MyMPR121::setThresholds(uint8_t touch, uint8_t release) {
  // set all thresholds (the same)
  for (uint8_t i = 0; i < 12; i++) {
    writeRegister(MPR121_TOUCHTH_0 + 2 * i, touch);
    writeRegister(MPR121_RELEASETH_0 + 2 * i, release);
  }
}

void MyMPR121::verifyRegisters() {
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

void MyMPR121::dumpCapData(uint8_t electrode) {
  if (electrode > 11)
    return;

  uint16_t filtered = filteredData(electrode);
  uint16_t baseline = baselineData(electrode);
  int16_t delta = (int16_t)filtered - (int16_t)baseline;

  /*
  wrote this before knowing about filteredData() and baselineData() methods,
  but it is still useful info so I'm just commenting it out
  */
  /*
  uint8_t lsb = readRegister8(0x04 + 2 * electrode); // will return 8 bits
  uint8_t msb = readRegister8(0x05 + 2 * electrode); // will return 2 bits
  uint16_t filtered =
      ((uint16_t)(msb & 0x03) << 8) | lsb; // combined into 10-bit value

  // read 8-bit baseline REGISTERS (these 8-bits acrually represent the upper 8
  // bits of a 10-bit system)
  uint8_t baseline_raw = readRegister8(0x1E + electrode);
  uint16_t baseline = baseline_raw
                      << 2; // left shift to match filtered data scale (10-bits)

  // calculate delta (filtered - baseline), cast to int16_t because delta can be
  // negative
  int16_t delta = (int16_t)filtered - (int16_t)baseline;
  */

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

void MyMPR121::dumpCDCandCDTRegisters() {
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
