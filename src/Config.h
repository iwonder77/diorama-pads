#ifndef CONFIG_H
#define CONFIG_H
/**
 * Config.h
 *
 * Centralized configuration constants used by the diorama touch pads system
 */

#include <Arduino.h>

namespace Config {
enum class AppState { DEBUG, RUN, ERROR_RECOVERY };
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 10000;

namespace Audio {
constexpr uint8_t AUDIO_RXI = 20; // ESP32C3 RXI -> DY-HV20T TX
constexpr uint8_t AUDIO_TXO = 21; // ESP32C3 TXO -> DY-HV20T RX
constexpr uint8_t AUDIO_BUSY = A4;
constexpr unsigned long AUDIO_BEGIN_TIMEOUT_MS = 150;
constexpr unsigned long AUDIO_END_COOLDOWN_MS = 100;
constexpr unsigned long AUDIO_MAX_DURATION_MS = 7000;
} // namespace Audio

namespace Touch {
constexpr uint8_t MPR121_I2C_ADDR = 0x5A;
constexpr uint8_t NUM_ELECTRODES = 3;

// --- MPR121 Threshold Constants ---
constexpr uint8_t TOUCH_THRESHOLD = 12;
constexpr uint8_t RELEASE_THRESHOLD = 6;

// --- FILTER/GLOBAL CONFIGURATION ---
// see sec 5.8 of datasheet for description and encoding values
// 0x5C: Filter/Global CDC Configuration Register (CONFIG1)
// * FFI (First Filter Iterations) - bits [7:6]
// * CDC (global Charge/Discharge Current) - bits [5:0]
// NOTE: FFI here must match FFI in AUTOCONFIG0 (0x7B) if set
constexpr uint8_t FFI = 0b10;            // 18 samples for first filter
constexpr uint8_t CDC_GLOBAL = 0b100000; // sets current to 32μA
constexpr uint8_t REG_CONFIG1 = (FFI << 6) | (CDC_GLOBAL & 0x3F);
// 0x5D: Filter/Global CDT Configuration Register (CONFIG2)
// * CDT (global Charge/Discharge Time) - bits [7:5]
// * SFI (Second Filter Iterations) - bits [4:3]
// * ESI (Electrode Sample Interval) - bits [2:0]
constexpr uint8_t CDT_GLOBAL = 0b010; // sets charge time to 1μS
constexpr uint8_t SFI = 0b10;         // 10 samples
constexpr uint8_t ESI = 0b010;        // 4 ms sampling period
constexpr uint8_t REG_CONFIG2 = (CDT_GLOBAL << 5) | (SFI << 3) | ESI;

// --- BASELINE TRACKING CONFIGURATION ---
// See sec 5.5 of datasheet AND application note
// AN3891 for baseline tracking and filtering info
//
// With the 0.2" PLA plate on the copper pads, the filtered data only dropped
// about ~30 counts, the MPR121's default FALLING baseline tracking treated
// this low delta as noise and quickly pulled the baseline down, making touches
// undetectable
//
// To fix this, we'll slow down the FALLING baseline tracking by adjusting:
// * MHD (Maximum Half Delta): largest variation allowed through baseline filter
// * NHD (Noise Half Delta): baseline step size when non-noise drift IS detected
// * NCL (Noise Count Limit): samples needed above MHD to treat changes as touch
// * FDL (Filter Delay Limit): controles baseline update speed (larger = slower)
constexpr uint8_t MHDF = 0x04; // tiny delta (±8 counts) are tracked immediately
constexpr uint8_t NHDF = 0x01; // tiny increment (1 count) when baseline adjusts
constexpr uint8_t NCLF = 0x90; // 144 successive samples beyond MHD b4 adjusting
constexpr uint8_t FDLF = 0x06; // slight filter delay to slow baseline system
// RISING baseline tracking
constexpr uint8_t MHDR = 0x01;
constexpr uint8_t NHDR = 0x01;
constexpr uint8_t NCLR = 0x04;
constexpr uint8_t FDLR = 0x00;

// --- ELECTRODE CONFIGURATION REGISTER (ECR) 0x5E ---
// see sec 5.11 of datasheet
// * CL (Calibration Lock) - bits [7:6], controls baseline tracking/init value
// * ELEPROX_EN - bits [5:4], enables and controls proximity detection
// * ELE_EN - bits [3:0], enables electrode touch/capacitance detection
constexpr uint8_t CL = 0b11; // init baseline loaded with 5 hb of 1st electrode
constexpr uint8_t ELEPROX_EN = 0b00;       // proximitiy detection disabled
constexpr uint8_t ELE_EN = NUM_ELECTRODES; // enable electrodes 0-x (run mode)
constexpr uint8_t REG_ECR_RUN = (CL << 6) | (ELEPROX_EN << 4) | ELE_EN;

// --- AUTO CONFIGURATION ---
// see pg 17 of datasheet for description and encoding values
// Auto-Configuration Control Register 0: 0x7B (AUTOCONFIG0)
//  * FFI (First Filter Iterations) - bits [7:6] (MUST BE SAME AS 0x5C)
//  * RETRY - bits [5:4]
//  * BVA - bits [3:2], fill with same bits as CL bits in ECR (0x5E) register
//  * ARE (Auto-Reconfiguration Enable) - bit 1
//  * ACE (Auto-Configuration Enable) - bit 0
constexpr uint8_t RETRY = 0b00;
constexpr uint8_t BVA = CL;
constexpr uint8_t ARE = 0;
constexpr uint8_t ACE = 0;
constexpr uint8_t REG_AUTOCONFIG0 =
    (FFI << 6) | (RETRY << 4) | (BVA << 2) | (ARE << 1) | ACE;
// Auto-Configuration Control Register 1: 0x7C (AUTOCONFIG1)
//  * SCTS (Skip Charge Time Search) - bit 7
//  * bits [6:3] unused
//  * OORIE (Out-of-range interrupt enable) - bit 2
//  * ARFIE (Auto-reconfiguration fail interrupt enable) - bit 1
//  * ACFIE (Auto-configuration fail interrupt enable) - bit 0

// --- LIMIT REGISTERS (all 8-bits) ---
// see pg 18 of datasheet AND application note 3889 for more information
// on these values
// Up-Side Limit Register: 0x7D (UPLIMIT)
//  * USL: sets electrode data level up limit for autoconfig boundary check
// Low-Side Limit Register: 0x7E (LOWLIMIT)
//  * LSL: sets electrode data level low limit for autoconfig boundary check
// Up-Side Limit Register: 0x7F (TARGETLIMIT)
//  * TL: expected target electrode data level for autoconfig
// (remember, formulas below assume Vdd = 3.3V)
constexpr uint8_t USL = 200; // ((Vdd - 0.7)/Vdd) * 256
constexpr uint8_t LSL = 130; // UPLIMIT * 0.65
constexpr uint8_t TL = 180;  // UPLIMIT * 0.9

// --- SOFTWARE TOUCH DETECTION ---
constexpr float ALPHA = 0.4f; // α used in the EMA filter formula
constexpr int16_t DELTA_TOUCH_THRESHOLD = -25;
constexpr int16_t DELTA_RELEASE_THRESHOLD = -15;
constexpr uint8_t DEBOUNCE_COUNT = 5;

} // namespace Touch
} // namespace Config
#endif
