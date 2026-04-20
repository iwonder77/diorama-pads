#ifndef SRC_NEOPIXELRING_H_
#define SRC_NEOPIXELRING_H_

#include <Adafruit_NeoPixel.h>

#include "Config.h"

class NeoPixelRing {
public:
  NeoPixelRing(uint8_t pin, uint16_t count, uint8_t brightness = 35,
               Config::Color base_color = {60, 20, 0, 180});

  void begin();
  void update();

  void setColor(const Config::Color &color);
  void setBrightness(uint8_t brightness);
  void setBreathPeriod(float period_ms);
  void setBreathMin(float min_scale);
  void setFps(uint8_t fps);

private:
  Adafruit_NeoPixel strip_;
  uint8_t brightness_;
  Config::Color base_color_;
  float breath_period_ms_;
  float breath_min_;
  uint32_t frame_ms_;
  uint32_t last_frame_;
};

#endif // SRC_NEOPIXELRING_H_
