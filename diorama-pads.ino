#include <Wire.h>
#include "src/MyMPR121.h"
#include "src/AudioPlayer.h"
// #include "src/RingWindow.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define RXI_PIN 20  // ESP32C3 RXI -> DY-HV20T TX
#define TXO_PIN 21  // ESP32C3 TXO -> DY-HV20T RX
#define BUSY_PIN A4

const uint8_t NUM_ELECTRODES = 1;
const uint8_t MPR121_I2C_ADDR = 0x5A;

uint16_t lastTouched = 0;
uint16_t currTouched = 0;

enum class SystemState {
  IDLE,
  PLAYING
};

MyMPR121 mpr121 = MyMPR121();
AudioPlayer player;

SystemState currentState = SystemState::IDLE;

void setup() {
  Wire.begin();
  delay(100);
  Serial.begin(9600);
  delay(100);
  Serial1.begin(9600, SERIAL_8N1, RXI_PIN, TXO_PIN);
  // while (!Serial);
  delay(3000);
  pinMode(BUSY_PIN, INPUT);

  if (!mpr121.begin(MPR121_I2C_ADDR, &Wire)) {
    Serial.print("MPR121 not found, check wiring");
    // hold indefinitely
    while (1);
  }

  Serial.println("Initial CDC and CDT Values:");
  mpr121.dumpCDCandCDTRegisters();
  delay(1000);

  mpr121.setAutoconfig(true);

  Serial.println("New CDC and CDT Values:");
  mpr121.dumpCDCandCDTRegisters();
  delay(1000);

  Serial.println("=== ELECTRODE READINGS ===");
  Serial.println("Electrode, Filtered, Baseline, Delta");
}

void loop() {
  // read busy pin
  bool isPlaying = (digitalRead(BUSY_PIN) == LOW);

  // touched method returns a 16-bit value where each bit represents one pad
  // so if pads 0 and 2 are currently touched:
  // currTouched = 0b0000_0000_0000_0101
  currTouched = mpr121.touched();

  switch (currentState) {
    case SystemState::IDLE:
      for (uint8_t i = 0; i < 12; i++) {
        // it if *is* touched and *wasnt* touched before, begin playing and immediately transition state!
        if ((currTouched & _BV(i)) && !(lastTouched & _BV(i))) {
          Serial.print(i);
          Serial.println(" touched");
          player.playTrackByIndex((uint16_t)i + 1);
          currentState = SystemState::PLAYING;
          break;
        }
      }
      break;
    case SystemState::PLAYING:
      if (!isPlaying) {
        // only transition back to IDLE state when sound is done playing
        currentState = SystemState::IDLE;
      } else {
        Serial.println("PLAYING");
      }
      break;
  }
  lastTouched = currTouched;

  // put a delay so it isn't overwhelming
  delay(100);
}
