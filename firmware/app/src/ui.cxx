#include <algorithm>
#include <stdint.h>
#include <vector>
#include <random>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/led_strip.h>

#include "ui.h"


LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

using namespace H4X;

namespace {
  UI::LED* led_ = nullptr;

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
    static struct k_spinlock rand32_lock;

    /* initial seed value */
    static uint64_t state = (uint64_t)1337;
    k_spinlock_key_t key = k_spin_lock(&rand32_lock);

    state = state + k_cycle_get_32();
    state = state * 2862933555777941757ULL + 3037000493ULL;
    uint32_t val = (uint32_t)(state >> 32);

    k_spin_unlock(&rand32_lock, key);
    return val;
  }
}

int UI::StaticLED::OnTrigger(size_t port)
{
  ARG_UNUSED(port);

  std::fill_n(&this->pixels_[0], this->size_, hexToRGB(CONFIG_APP_UI_STATIC_LED_COLOR));
  return led_strip_update_rgb(this->dev_, this->pixels_, this->size_);
}

void UI::StaticLED::clear()
{
  std::fill_n(&this->pixels_[0], this->size_, led_rgb{0, 0, 0});
}

int UI::MosaicLED::OnTrigger(size_t port)
{
  if (this->freePixels_.size() == 0) {
    this->fillFreePixels();
  }

  size_t ledNum = this->freePixels_.back();
  this->freePixels_.pop_back();
  this->pixels_[ledNum] = hexToRGB(kMosaicPalette[port % kMosaicPalette.size()]);

  return led_strip_update_rgb(this->dev_, this->pixels_, this->size_);
}

void UI::MosaicLED::fillFreePixels()
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

void UI::OnTrigger(size_t port)
{
  led_->OnTrigger(port);
}

int UI::Init()
{
  const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ui_led));
  int err = 0;
  if (!device_is_ready(dev)) {
    LOG_ERR("LED device %s is not ready", dev->name);
    return err;
  }

  size_t numLeds =  DT_PROP(DT_ALIAS(ui_led), chain_length);
  LOG_INF("Found strip device %s with %d leds", dev->name, numLeds);

#if CONFIG_APP_UI_LED_STATIC
  led_ = new UI::StaticLED(dev, numLeds);
  err = led_->OnTrigger(0);
  if (err) {
    LOG_ERR("couldn't turn on LED: %d", err);
    return err;
  }

#elif CONFIG_APP_UI_LED_MOSAIC
  led_ = new UI::MosaicLED(dev, numLeds);
#else
  LOG_ERR("no led pattern configured :/");
#endif

  return 0;
}
