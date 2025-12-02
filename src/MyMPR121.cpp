#include "MyMPR121.h"
#include "Adafruit_MPR121.h"

MyMPR121::MyMPR121() : Adafruit_MPR121() {}

bool MyMPR121::begin(uint8_t i2c_addr, TwoWire *the_wire) {
  // First, do the standard Adafruit initialization
  // This sets up I2C, does soft reset, puts in STOP mode
  if (!Adafruit_MPR121::begin(i2c_addr, the_wire,
                              MPR121_TOUCH_THRESHOLD_DEFAULT,
                              MPR121_RELEASE_THRESHOLD_DEFAULT, false)) {
    return false;
  }

  // ---------- SET BASELINE TRACKING PARAMETERS ----------
  // see page 12 (sec 5.5) of datasheet AND application
  // note AN3891 for baseline tracking and filtering info
  //
  // with the 0.2" PLA plate on the copper touch pads, the delta between
  // filtered data and baseline was quite small (around -30, BEFORE any changes
  // were made to signal strength, i.e. charge/discharge current and time)
  //
  // the MPR121's default baseline tracking system parameters for this FALLING
  // case led the baseline to immediately track the filtered data (believing it
  // was noise/changes to environment instead of an actual touch event) this
  // rendered the pads useless in detecting touches
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
  // ----------------------------------------------------------------------------

  // --- DEBOUNCE ---
  writeRegister(MPR121_DEBOUNCE, 0);

  // ---------- CONFIG1 & CONFIG 2 ----------
  // (see pg 14 of datasheet for description and encoding values)
  // Filter/Global CDC Configuration Register: 0x5C (CONFIG1)
  // set to 0x10 = 0b00110000
  //  * FFI (First Filter Iterations) - bits [7:6]
  uint8_t ffi = 0b00; // sets samples taken to 6 (default)
  //  * CDC (global Charge/Discharge Current) - bits [5:0]
  uint8_t cdc_global = 0b100000; // sets current to 32μA
  uint8_t cfg1_reg = (ffi << 6) | (cdc_global & 0x3F);
  writeRegister(MPR121_CONFIG1, cfg1_reg);
  // Filter/Global CDT Configuration Register: 0x5D (CONFIG2)
  // set to 0x41 = 0b01000001
  //  * CDT (global Charge/Discharge Time) - bits [7:5]
  uint8_t cdt_global = 0b010; // sets charge time to 1μS
  //  * SFI (Second Filter Iterations) - bits [4:3]
  uint8_t sfi = 0b00; // number of samples for 2nd level filter set to 4
  //  * ESI (Electrode Sample Interval) - bits [2:0]
  uint8_t esi = 0b001; // electrode sampling period set to 2 ms
  uint8_t cfg2_reg = (cdt_global << 5) | (sfi << 3) | esi;
  writeRegister(MPR121_CONFIG2, cfg2_reg);

  // ---------- AUTOCONFIG ----------
  // important: disable autoconfig so it doesn't overwrite our CDC/CDT values
  // Auto-Configuration Control Register 0: 0x7B (AUTOCONFIG0)
  //  * FFI (First Filter Iterations) - bits [7:6] (same as 0x5C)
  //  * RETRY - bits [5:4]
  //  * BVA - bits [3:2], fill with same bits as CL bits in ECR (0x5E) register
  //  * ARE (Auto-Reconfiguration Enable) - bit 1
  //  * ACE (Auto-Configuration Enable) - bit 0
  writeRegister(MPR121_AUTOCONFIG0, 0b00001010);
  // Auto-Configuration Control Register 1: 0x7C (AUTOCONFIG1)
  //  * SCTS (Skip Charge Time Search) - bit 7
  //  * bits [6:3] unused
  //  * OORIE (Out-of-range interrupt enable) - bit 2
  //  * ARFIE (Auto-reconfiguration fail interrupt enable) - bit 1
  //  * ACFIE (Auto-configuration fail interrupt enable) - bit 0
  // (see pg 17 of datasheet for description and encoding values)

  // ---------- RUN Mode ----------
  // this is already set by parent `begin()` function, but has useful info so I
  // left it
  // Set ECR to enable all 12 electrodes
  //  * CL (Calibration Lock) - bits [7:6] = 0b10 (baseline tracking enabled
  //  with 5 bits)
  //  * ELEPROX_EN (Proximity Enable) - bits [5:4] = 0b00 (proximity detection
  //  disabled)
  //  * ELE_EN (Electrode Enable) - bits [3:0] = 0b1100 (run mode with electrode
  //  0 - 11 detection enabled)
  // writeRegister(MPR121_ECR, 0x8C);

  return true;
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
