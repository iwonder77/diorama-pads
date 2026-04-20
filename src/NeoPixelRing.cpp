#include "NeoPixelRing.h"

#include <Arduino.h>

NeoPixelRing::NeoPixelRing(uint8_t pin, uint16_t count, uint8_t brightness,
                           Config::Color base_color)
    : strip_(count, pin, NEO_GRBW + NEO_KHZ800), brightness_(brightness),
      base_color_(base_color), breath_period_ms_(3000.0f), breath_min_(0.08f),
      frame_ms_(1000 / 60), last_frame_(0) {}

void NeoPixelRing::begin() {
  strip_.begin();
  strip_.setBrightness(brightness_);
  strip_.show();
}

void NeoPixelRing::update() {
  uint32_t now = millis();
  if (now - last_frame_ < frame_ms_)
    return;
  last_frame_ = now;

  float scale =
      breath_min_ + (1.0f - breath_min_) *
                        (sinf(now / breath_period_ms_ * TWO_PI) + 1.0f) * 0.5f;

  strip_.fill(strip_.Color(
      (uint8_t)(base_color_.r * scale), (uint8_t)(base_color_.g * scale),
      (uint8_t)(base_color_.b * scale), (uint8_t)(base_color_.w * scale)));
  strip_.show();
}

void NeoPixelRing::setColor(const Config::Color &color) { base_color_ = color; }

void NeoPixelRing::setBrightness(uint8_t brightness) {
  brightness_ = brightness;
  strip_.setBrightness(brightness_);
}

void NeoPixelRing::setBreathPeriod(float period_ms) {
  breath_period_ms_ = period_ms;
}

void NeoPixelRing::setBreathMin(float min_scale) { breath_min_ = min_scale; }

void NeoPixelRing::setFps(uint8_t fps) { frame_ms_ = 1000 / fps; }
