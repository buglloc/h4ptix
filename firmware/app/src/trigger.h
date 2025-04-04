#pragma once

#include <zephyr/kernel.h>

#include <app/drivers/gpio_trigger.h>

namespace H4X::Trigger {

  enum ErrorCode : uint8_t {
    None = 0,
    PortBusy,
    PortInvalid,
    Internal
  };

  class Port {
  public:
    Port(const struct device *dev, size_t port);

    ErrorCode Trigger(size_t duration, size_t delay);
    ErrorCode On(size_t duration);
    ErrorCode Off();

  private:
    const struct device *dev_;
    size_t port_;
    size_t duration_;
    atomic_t state_;
    struct k_work_delayable dwork_;
  };

  int Init();
  size_t Size();
  ErrorCode Trigger(size_t port, size_t duration = CONFIG_APP_TRIGGER_DEFAULT_DURATION, size_t delay = 0);
}
