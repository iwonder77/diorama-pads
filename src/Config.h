#ifndef CONFIG_H
#define CONFIG_H
/**
 * Config.h
 *
 * Centralized configuration constants used by the diorama touch pads system
 */

#include <Arduino.h>

namespace Config {
enum class AppState { DEBUG, RUN_PROD, ERROR_RECOVERY };

namespace Audio {
constexpr uint8_t AUDIO_RXI = 20; // ESP32C3 RXI -> DY-HV20T TX
constexpr uint8_t AUDIO_TXO = 21; // ESP32C3 TXO -> DY-HV20T RX
constexpr uint8_t AUDIO_BUSY = A4;
} // namespace Audio

namespace Touch {
constexpr uint8_t MPR121_I2C_ADDR = 0x5A;
constexpr uint8_t NUM_ELECTRODES = 3;
constexpr uint8_t NUM_AVALIABLE_ELECTRODES = 12;

// --- MPR121 Threshold Constants ---
constexpr uint8_t TOUCH_THRESHOLD = 12;
constexpr uint8_t RELEASE_THRESHOLD = 6;

// --- FILTER/GLOBAL CONFIGURATION ---
// (see pg 14 of datasheet for description and encoding values)
// 0x5C: Filter/Global CDC Configuration Register (CONFIG1)
// * FFI (First Filter Iterations) - bits [7:6]
// * CDC (global Charge/Discharge Current) - bits [5:0]
constexpr uint8_t FFI = 0b01;            // 10 samples for first filter
constexpr uint8_t CDC_GLOBAL = 0b100000; // sets current to 32μA
constexpr uint8_t REG_CONFIG1 = (FFI << 6) | (CDC_GLOBAL & 0x3F);
// 0x5D: Filter/Global CDT Configuration Register (CONFIG2)
// * CDT (global Charge/Discharge Time) - bits [7:5]
// * SFI (Second Filter Iterations) - bits [4:3]
// * ESI (Electrode Sample Interval) - bits [2:0]
constexpr uint8_t CDT_GLOBAL = 0b010; // sets charge time to 1μS
constexpr uint8_t SFI = 0b01;         // 6 samples
constexpr uint8_t ESI = 0b000;        // 1 ms sampling period
constexpr uint8_t REG_CONFIG2 = (CDT_GLOBAL << 5) | (SFI << 3) | ESI;

// --- BASELINE TRACKING CONFIGURATION ---
// See page 12 (sec 5.5) of datasheet AND application
// note AN3891 for baseline tracking and filtering info
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

// --- SOFTWARE TOUCH DETECTION ---
constexpr float ALPHA = 0.4f; // α used in the EMA filter formula
constexpr int16_t DELTA_TOUCH_THRESHOLD = -20;
constexpr int16_t DELTA_RELEASE_THRESHOLD = -10;
constexpr uint8_t DEBOUNCE_COUNT = 5;

} // namespace Touch
} // namespace Config
#endif
