#pragma once
#include <algorithm>
#include <vector>

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>

namespace H4X::UI {

  class LED {
  public:
    virtual ~LED() = default;
    virtual int OnTrigger(size_t port) = 0;
  };

  class StaticLED: public LED {
  public:
    StaticLED(const struct device *dev, size_t numLeds)
      : dev_(dev)
      , size_(numLeds)
    {
      this->pixels_ = new struct led_rgb[this->size_];
      this->clear();
    }

    ~StaticLED()
    {
      delete[] this->pixels_;
    }

    int OnTrigger(size_t port) override;

  protected:
    void clear();

  protected:
    const struct device *dev_;
    size_t size_;
    struct led_rgb *pixels_{nullptr};
  };

  class MosaicLED: public StaticLED {
  public:
    MosaicLED(const struct device *dev, size_t numLeds)
      : StaticLED(dev, numLeds)
    {
    }

    ~MosaicLED() = default;
    int OnTrigger(size_t port) override;

  protected:
    void fillFreePixels();

  protected:
    std::vector<size_t> freePixels_{};
  };

  int Init();
  void OnTrigger(size_t port);
}
