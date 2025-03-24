#include <algorithm>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/led_strip.h>

#include "led.h"


LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

using namespace H4X::UI;

namespace {
  struct led_rgb hexToRGB(uint32_t c)
  {
    return {
      .r = static_cast<uint8_t>(((c >> 16) & 0xFF) * CONFIG_APP_UI_LED_BRIGHTNESS / 100),
      .g = static_cast<uint8_t>(((c >> 8)  & 0xFF) * CONFIG_APP_UI_LED_BRIGHTNESS / 100),
      .b = static_cast<uint8_t>(((c >> 0)  & 0xFF) * CONFIG_APP_UI_LED_BRIGHTNESS / 100),
    };
  }
}

StaticLED::StaticLED(const struct device *dev, size_t numLeds)
: dev_(dev)
, size_(numLeds)
{
  this->pixels_ = new struct led_rgb[this->size_];
  int err = this->Set(CONFIG_APP_UI_LED_INACTIVE_COLOR);
  if (err) {
    LOG_ERR("Unable to set initial LED state: %d", err);
  }
}

int StaticLED::Set(uint32_t color)
{
  std::fill_n(&this->pixels_[0], this->size_, hexToRGB(color));
  return led_strip_update_rgb(this->dev_, this->pixels_, this->size_);
}

int StaticLED::TriggerOn(size_t port)
{
  ARG_UNUSED(port);
  return 0;
}

int StaticLED::TriggerOff(size_t port)
{
  ARG_UNUSED(port);
  return 0;
}

int BlinkyLED::TriggerOn(size_t port)
{
  ARG_UNUSED(port);
  return this->Set(CONFIG_APP_UI_LED_ACTIVE_COLOR);
}

int BlinkyLED::TriggerOff(size_t port)
{
  ARG_UNUSED(port);
  return this->Set(CONFIG_APP_UI_LED_INACTIVE_COLOR);
}
