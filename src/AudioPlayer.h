#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H
/**
 * AudioPlayer.h
 *
 * UART control wrapper for communicating with the DY-HV20T module
 *  - protocol structure:
 * ┌────────────┬─────┬──────────┬─────────────────┬──────────┐
 * │ START_BYTE │ CMD │ DATA_LEN │  DATA BYTES...  │ CHECKSUM │
 * │   0xAA     │     │          │                 │          │
 * └────────────┴─────┴──────────┴─────────────────┴──────────┘
 */

#include <Arduino.h>

#define START_BYTE 0xAA
#define CMD_SPECIFIED_SONG 0x07
#define CMD_PLAY 0x02
#define CMD_PAUSE 0x03
#define CMD_STOP 0x04

constexpr uint8_t MAX_CMD_SIZE = 5;

class AudioPlayer {
public:
  void playTrackByIndex(uint16_t index);
  void stop();
  void pause();

private:
  void sendCommand(uint8_t cmd, uint8_t dataLen, uint8_t *data);
  uint8_t calculateChecksum(uint8_t *buffer, uint8_t len);
};

#endif
