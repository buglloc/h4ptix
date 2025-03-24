#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/led_strip.h>


namespace H4X::UI {

  class LED {
  public:
    virtual ~LED() = default;
    virtual int TriggerOn(size_t port) = 0;
    virtual int TriggerOff(size_t port) = 0;
  };

  class StaticLED: public LED {
  public:
    StaticLED(const struct device *dev, size_t numLeds);
    ~StaticLED()
    {
      delete[] this->pixels_;
    }

    virtual int TriggerOn(size_t port) override;
    virtual int TriggerOff(size_t port) override;

  protected:
    int Set(uint32_t color);

  protected:
    const struct device *dev_;
    size_t size_;
    struct led_rgb *pixels_{nullptr};
  };

  class BlinkyLED: public StaticLED {
  public:
  BlinkyLED(const struct device *dev, size_t numLeds)
      : StaticLED(dev, numLeds)
    {}

    ~BlinkyLED() = default;
    virtual int TriggerOn(size_t port) override;
    virtual int TriggerOff(size_t port) override;
  };
}
