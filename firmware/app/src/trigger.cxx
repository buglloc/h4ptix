#include <vector>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic_c.h>
#include <zephyr/zbus/zbus.h>

#include "app/drivers/gpio_trigger.h"

#include "trigger.h"
#include "messages.h"


LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

ZBUS_CHAN_DECLARE(trigger_chan);

using namespace H4X;

namespace {
  std::vector<Trigger::Port> ports_{};
}

int Trigger::Init()
{
  const struct device *dev;
  LOG_INF("initialize trigger");

  dev = DEVICE_DT_GET(DT_NODELABEL(triggers));
  if (!device_is_ready(dev)) {
    LOG_ERR("Trigger dev not ready");
    return -ENODEV;
  }

  size_t maxPort = gpio_trigger_size(dev);
  for (size_t port = 0; port < maxPort; ++port) {
    ports_.emplace_back(dev, port);
  }

  return 0;
}

size_t Trigger::Size()
{
  return ports_.size();
}

Trigger::ErrorCode Trigger::Trigger(size_t port, size_t duration)
{
  if (port >= ports_.size()) {
    return Trigger::ErrorCode::PortInvalid;
  }

  return ports_[port].On(duration > 0 ? duration : CONFIG_APP_TRIGGER_DEFAULT_DURATION);
}

Trigger::Port::Port(const struct device *dev, size_t port)
  : dev_(dev)
  , port_(port)
  , busy_(ATOMIC_INIT(0))
{
  k_work_init_delayable(&this->dwork_, [](struct k_work* const item) {
    struct k_work_delayable* dwork = k_work_delayable_from_work(item);
    Port* port = CONTAINER_OF(dwork, Port, dwork_);
    port->Off();
  });
};

Trigger::ErrorCode Trigger::Port::On(size_t duration)
{
  if (atomic_set(&this->busy_, 1) == 1) {
    return Trigger::ErrorCode::PortBusy;
  }

  int err = gpio_trigger_on(this->dev_, this->port_);
  if (err) {
    LOG_ERR("unable to activate port#%d: %d", this->port_, err);
    return Trigger::ErrorCode::Internal;
  }

  TriggerMsg msg = {
    .Port = this->port_,
    .On = true,
  };
  zbus_chan_pub(&trigger_chan, &msg, K_MSEC(CONFIG_APP_TRIGGER_CHAN_PUB_TIMEOUT));
  k_work_schedule(&this->dwork_, K_MSEC(duration));
  return Trigger::ErrorCode::None;
}

Trigger::ErrorCode Trigger::Port::Off()
{
  if (atomic_set(&this->busy_, 0) == 0) {
    return Trigger::ErrorCode::None;
  }

  TriggerMsg msg = {
    .Port = this->port_,
    .On = false,
  };
  zbus_chan_pub(&trigger_chan, &msg, K_MSEC(CONFIG_APP_TRIGGER_CHAN_PUB_TIMEOUT));

  int err = gpio_trigger_off(this->dev_, this->port_);
  if (err) {
    LOG_ERR("unable to deactivate port#%d: %d", this->port_, err);
    return Trigger::ErrorCode::Internal;
  }

  return ErrorCode::None;
}
