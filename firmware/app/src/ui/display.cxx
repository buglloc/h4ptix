#include "display.h"

#include <cstddef>
#include <cstdio>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/display/cfb.h>

#include "cfb_ui.h"


LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

using namespace H4X::UI;

namespace {
  const char* kTitle = "~ H4ptiX ~";
  const uint8_t kFontKerning = 0;
  const uint16_t kSpacing = 2;
  constexpr size_t kMaxCnt = 0xffffff;
};

int Display::TriggerOn(size_t port)
{
  ARG_UNUSED(port);
  return this->Draw(true);
}

int Display::TriggerOff(size_t port)
{
  if (this->cnt_ > kMaxCnt) {
    this->cnt_ = 0;
  }
  this->cnt_++;

  return this->Draw(false);
}

int Display::Initialize()
{
  int err = 0;
  err = display_set_pixel_format(this->dev_, PIXEL_FORMAT_MONO10);
	if (err) {
    err = display_set_pixel_format(this->dev_, PIXEL_FORMAT_MONO01);
		if (err) {
      LOG_ERR("Failed to set required pixel format: %d", err);
			return err;
		}
	}

  err = display_blanking_off(this->dev_);
  if (err) {
		LOG_ERR("Failed to turn off display blanking: %d", err);
		return err;
	}

  err = cfb_framebuffer_init(this->dev_);
	if (err) {
    LOG_ERR("Could not initialize framebuffer: %d", err);
    return err;
	}

  cfb_set_kerning(this->dev_, kFontKerning);
  this->width_ = cfb_get_display_parameter(this->dev_, CFB_DISPLAY_WIDTH);
  this->height_ = cfb_get_display_parameter(this->dev_, CFB_DISPLAY_HEIGH);
  err = cfb_get_font_size(this->dev_, 0, &this->font_width_, &this->font_height_);
	if (err) {
    LOG_ERR("Could not get font dimensions: %d", err);
    return err;
	}

  return this->Draw(false);
}

int Display::Draw(bool on)
{
  int err = this->Clear();
  if (err) {
    return err;
  }

  uint16_t y = this->DrawHeader(on);

  char buf[128];
  std::snprintf(buf, sizeof(buf), "0x%06X", this->cnt_);

  y += (this->height_ - y) / 2 - this->font_height_ / 2;
  uint16_t cntX = this->font_width_ * strlen(buf);
  cfb_draw_text(
    this->dev_,
    buf,
    cntX < this->width_ ? (this->width_ - cntX) / 2 : 0,
    y
  );

  return this->Flush();
}

uint16_t Display::DrawHeader(bool on)
{
  uint16_t y = kSpacing;
  uint16_t x = this->font_width_ * strlen(kTitle);
  int err = cfb_draw_text(
    this->dev_,
    kTitle,
    x < this->width_ ? (this->width_ - x) / 2 : 0,
    y
  );
  if (err) {
    return 0;
  }

  y +=  this->font_height_;
  y += kSpacing;

  if (on) {
    err = cfb_invert_area(this->dev_, 0, 0, this->width_, y);
    if (err) {
      return 0;
    }
  }

  struct cfb_position start = {
    .x = 0,
    .y = y,
  };

  struct cfb_position end = {
    .x = this->width_,
    .y = start.y,
  };

  err = cfb_draw_line(this->dev_, &start, &end);
  if (err) {
    return 0;
  }

  return y + kSpacing;
}

int Display::Clear()
{
  int err = cfb_framebuffer_clear(this->dev_, true);
	if (err) {
    LOG_ERR("Could not clear framebuffer: %d", err);
	}

  return err;
}

int Display::Flush()
{
  int err = cfb_framebuffer_finalize(this->dev_);
	if (err) {
    LOG_ERR("Could not finalize framebuffer: %d", err);
	}

  return err;
}

Display::~Display()
{
  cfb_framebuffer_deinit(this->dev_);
}
