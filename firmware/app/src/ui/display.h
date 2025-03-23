#pragma once
#include <cstddef>

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>


namespace H4X::UI {

  class Display {
  public:
    Display(const struct device *dev, size_t numPorts)
    : dev_(dev)
    , numPorts_(numPorts)
    {};

    ~Display();

    int Initialize();
    int TriggerOn(size_t port);
    int TriggerOff(size_t port);

  protected:
    int Draw(bool on);
    uint16_t DrawHeader(bool on);
    int Clear();
    int Flush();

  protected:
    const struct device *dev_{nullptr};
    size_t numPorts_{0};
    size_t cnt_{0};
    uint16_t width_{0};
    uint16_t height_{0};
    uint8_t font_width_{0};
    uint8_t font_height_{0};
  };

}
