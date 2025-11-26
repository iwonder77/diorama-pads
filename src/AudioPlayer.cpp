#include "AudioPlayer.h"

void AudioPlayer::playTrackByIndex(uint16_t index) {
  Serial.print("Playing track: ");
  Serial.println(index);

  // Format for "specified song" command is
  // 0xAA 0x07 0x02 [S.N.H.] [S.N.L.] [checksum]
  // where S.N.H. and S.N.L. are Song Number High/Low bytes, build them here
  uint8_t data[2];
  data[0] = (index >> 8) & 0xFF; // upper 8 bits
  data[1] = index & 0xFF;        // lower 8 bits

  sendCommand(CMD_SPECIFIED_SONG, 0x02, data);
}

void AudioPlayer::stop() { sendCommand(CMD_STOP, 0x00, nullptr); }

/**
 * @brief Function that builds the packet sent over UART with appropriate
 * checksum byte Format: [0xAA][CMD][DATA_LEN][DATA_BYTE_1]...[DATA_BYTE_N][CHK]
 *
 * @param cmd The command type byte
 * @param dataLen The number of bytes of data in command (if any)
 * @param data The data bytes for command (can be nullptr if dataLen is 0)
 */
void AudioPlayer::sendCommand(uint8_t cmd, uint8_t dataLen, uint8_t *data) {
  // prevent buffer overflow
  if (dataLen > MAX_CMD_SIZE - 3) {
    Serial.println("ERROR: dataLen exceeds buffer capacity");
    return;
  }

  // now build buffer for checksum calc
  uint8_t buffer[MAX_CMD_SIZE];
  buffer[0] = START_BYTE;
  buffer[1] = cmd;
  buffer[2] = dataLen;
  if (data != nullptr) {
    for (int i = 0; i < dataLen; i++) {
      buffer[3 + i] = data[i];
    }
  }
  uint8_t checksum = calculateChecksum(buffer, 3 + dataLen);

  // now send over UART
  Serial1.write(START_BYTE);
  Serial1.write(cmd);
  Serial1.write(dataLen);
  if (data != nullptr) {
    for (int i = 0; i < dataLen; i++) {
      Serial1.write(data[i]);
    }
  }
  Serial1.write(checksum);
}

uint8_t AudioPlayer::calculateChecksum(uint8_t *buffer, uint8_t len) {
  int sum = 0;
  for (int i = 0; i < len; i++) {
    sum += buffer[i];
  }
  return sum & 0xFF;
}
