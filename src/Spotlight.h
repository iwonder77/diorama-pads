#include <Arduino.h>

#include "Config.h"

namespace Spotlight {
void init();
void on(uint8_t index);
void off(uint8_t index);
void allOff();
} // namespace Spotlight
