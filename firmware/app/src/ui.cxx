#include "ui.h"
#include <cerrno>
#include <cstddef>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "ui/led.h"
#include "ui/display.h"

#include "messages.h"


LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

ZBUS_SUBSCRIBER_DEFINE(ui_trigger_subscriber, CONFIG_APP_UI_SUB_QUEUE_SIZE);

using namespace H4X;

namespace {
  UI::LED* led_ = nullptr;
#if CONFIG_DISPLAY
  UI::Display* dsp_ = nullptr;
#endif

  void ledWork(const TriggerMsg& msg)
  {
    if (led_ == nullptr) {
      return;
    }

    int err = msg.On ? led_->TriggerOn(msg.Port) : led_->TriggerOff(msg.Port);
    if (err) {
      LOG_ERR("Unable to update LED by trigger: %d", err);
    }
  }

  void displayWork(const TriggerMsg& msg)
  {
  #if CONFIG_DISPLAY
    if (dsp_ == nullptr) {
      return;
    }

    int err = msg.On ? dsp_->TriggerOn(msg.Port) : dsp_->TriggerOff(msg.Port);
    if (err) {
      LOG_ERR("Unable to update Display by trigger: %d", err);
    }
  #endif
  }

  void uiThreadTask(void)
  {
    const struct zbus_channel *chan;

    while (!zbus_sub_wait(&ui_trigger_subscriber, &chan, K_FOREVER)) {
      TriggerMsg msg;
      zbus_chan_read(chan, &msg, K_MSEC(CONFIG_APP_TRIGGER_CHAN_PUB_TIMEOUT));

      ledWork(msg);
      displayWork(msg);
    }
  }

  int InitLed()
  {
    const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ui_led));
    if (!device_is_ready(dev)) {
      LOG_ERR("LED device %s not ready", dev->name);
      return -ENODEV;
    }

    size_t numLeds =  DT_PROP(DT_ALIAS(ui_led), chain_length);
    LOG_INF("Found strip device %s with %d leds", dev->name, numLeds);

  #if CONFIG_APP_UI_LED_STATIC
    led_ = new UI::StaticLED(dev, numLeds);
  #elif CONFIG_APP_UI_LED_MOSAIC
    led_ = new UI::MosaicLED(dev, numLeds);
  #else
    LOG_ERR("no led pattern configured :/");
  #endif

    int err = led_->TriggerOn(0);
    if (err) {
      LOG_ERR("Failed to initialize LED: %d", err);
      return err;
    }

    return 0;
  }

  int InitDisplay(size_t numPorts)
  {
  #if CONFIG_DISPLAY
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(dev)) {
      LOG_ERR("Display device %s not ready", dev->name);
      return -ENODEV;
    }

    LOG_INF("Found display %s", dev->name);
    dsp_ = new UI::Display(dev, numPorts);
    int err = dsp_->Initialize();
    if (err) {
      LOG_ERR("Failed to initialize display: %d", err);
      return err;
    }
  #else
    LOG_INF("No display configured - skip it");
  #endif

    return 0;
  }
}

int UI::Init(size_t numPorts)
{
  int err = InitLed();
  if (err) {
    return err;
  }

  err = InitDisplay(numPorts);
  if (err) {
    return err;
  }

  return 0;
}

K_THREAD_DEFINE(ui_tid_, CONFIG_APP_UI_THREAD_STACK_SIZE, uiThreadTask, NULL, NULL, NULL, CONFIG_APP_UI_THREAD_PRIORITY, 0, 0);
