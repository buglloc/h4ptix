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

#if CONFIG_DISPLAY
#include <zephyr/display/cfb.h>
#endif

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

static int lala()
{
  const struct device *dev;
	uint16_t x_res;
	uint16_t y_res;
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(dev)) {
		printf("Device %s not ready\n", dev->name);
		return 0;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO01) != 0) {
			printf("Failed to set required pixel format");
			return 0;
		}
	}

	printf("Initialized %s\n", dev->name);

	if (cfb_framebuffer_init(dev)) {
		printf("Framebuffer initialization failed!\n");
		return 0;
	}

	cfb_framebuffer_clear(dev, true);

	display_blanking_off(dev);

	x_res = cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH);
	y_res = cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH);
	rows = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	for (int idx = 0; idx < 16; idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(dev, idx);
		printf("font width %d, font height %d\n",
		       font_width, font_height);
	}

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       x_res,
	       y_res,
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	cfb_framebuffer_invert(dev);

	cfb_set_kerning(dev, 1);

    cfb_framebuffer_clear(dev, false);
    if (cfb_print(dev,"0123456789mMgj!\"§$%&/()=0123456789mMgj!\"§$%&/()=0123456789mMgj!\"§$%&/()=",0, 0)) {
      printf("Failed to print a string\n");
      return 1;
    }
    cfb_framebuffer_finalize(dev);

  return 0;
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

  lala();
  return 0;
}
