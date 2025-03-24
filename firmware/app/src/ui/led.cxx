#include <algorithm>
#include <stdint.h>
#include <vector>
#include <random>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/led_strip.h>

#include "led.h"


LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

using namespace H4X::UI;

namespace {
  std::vector<uint32_t> kMosaicPalette = {
    0x00309a,
    0xec754a,
    0xd70029,
    0x3b591e,
    0x5d194d,
    0x2d709f,
    0xc29d93,
  };

  struct led_rgb hexToRGB(uint32_t c)
  {
    return {
      .r = static_cast<uint8_t>(((c >> 16) & 0xFF) * CONFIG_APP_UI_LED_BRIGHTNESS / 100),
      .g = static_cast<uint8_t>(((c >> 8)  & 0xFF) * CONFIG_APP_UI_LED_BRIGHTNESS / 100),
      .b = static_cast<uint8_t>(((c >> 0)  & 0xFF) * CONFIG_APP_UI_LED_BRIGHTNESS / 100),
    };
  }

  inline uint32_t rand32(void)
  {
    static uint64_t state = (uint64_t)1337;

    state = state + k_cycle_get_32();
    state = state * 2862933555777941757ULL + 3037000493ULL;
    return (uint32_t)(state >> 32);
  }
}

int StaticLED::TriggerOn(size_t port)
{
  ARG_UNUSED(port);

  std::fill_n(&this->pixels_[0], this->size_, hexToRGB(CONFIG_APP_UI_STATIC_LED_ACTIVE_COLOR));
  return led_strip_update_rgb(this->dev_, this->pixels_, this->size_);
}

int StaticLED::TriggerOff(size_t port)
{
  ARG_UNUSED(port);

  std::fill_n(&this->pixels_[0], this->size_, hexToRGB(CONFIG_APP_UI_STATIC_LED_INACTIVE_COLOR));
  return led_strip_update_rgb(this->dev_, this->pixels_, this->size_);
}

void StaticLED::clear()
{
  std::fill_n(&this->pixels_[0], this->size_, led_rgb{0, 0, 0});
}

int MosaicLED::TriggerOn(size_t port)
{
  ARG_UNUSED(port);
  return 0;
}

int MosaicLED::TriggerOff(size_t port)
{
  if (this->freePixels_.size() == 0) {
    this->fillFreePixels();
  }

  size_t ledNum = this->freePixels_.back();
  this->freePixels_.pop_back();
  this->pixels_[ledNum] = hexToRGB(kMosaicPalette[port % kMosaicPalette.size()]);

  return led_strip_update_rgb(this->dev_, this->pixels_, this->size_);
}

void MosaicLED::fillFreePixels()
{
  this->freePixels_.resize(this->size_);
  for (size_t i = 0; i < this->size_; ++i) {
    this->freePixels_[i] = i;
  }

  std::shuffle(
    this->freePixels_.begin(),
    this->freePixels_.end(),
    std::default_random_engine(rand32())
  );
}
