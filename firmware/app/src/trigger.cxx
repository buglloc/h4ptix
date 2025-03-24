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

  constexpr size_t kStateIdle = 0;
  constexpr size_t kStateWait = 1;
  constexpr size_t kStateBusy = 2;
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

Trigger::ErrorCode Trigger::Trigger(size_t port, size_t duration, size_t delay)
{
  if (port >= ports_.size()) {
    return Trigger::ErrorCode::PortInvalid;
  }

  if (duration == 0) {
    duration = CONFIG_APP_TRIGGER_DEFAULT_DURATION;
  }

  return ports_[port].Trigger(duration, delay);
}

Trigger::Port::Port(const struct device *dev, size_t port)
  : dev_(dev)
  , port_(port)
  , duration_(0)
  , state_(ATOMIC_INIT(kStateIdle))
{
  k_work_init_delayable(&this->dwork_, [](struct k_work* const item) {
    struct k_work_delayable* dwork = k_work_delayable_from_work(item);
    Port* port = CONTAINER_OF(dwork, Port, dwork_);
    int state = atomic_get(&port->state_);
    switch (state) {
    case kStateIdle:
      return;
    case kStateWait:
      port->On(port->duration_);
      break;
    case kStateBusy:
      port->Off();
      break;
    default:
      LOG_ERR("unexpected state: %d", state);
      break;
    }
  });

  LOG_INF("Trigger port#%d initialized", this->port_);
};

Trigger::ErrorCode Trigger::Port::Trigger(size_t duration, size_t delay)
{
  if (delay == 0) {
    return this->On(duration);
  }

  if (!atomic_cas(&this->state_, kStateIdle, kStateWait)) {
    return Trigger::ErrorCode::PortBusy;
  }

  this->duration_ = duration;
  k_work_schedule(&this->dwork_, K_MSEC(delay));
  return Trigger::ErrorCode::None;
}

Trigger::ErrorCode Trigger::Port::On(size_t duration)
{
  if (atomic_set(&this->state_, kStateBusy) == kStateBusy) {
    return Trigger::ErrorCode::PortBusy;
  }

  int err = gpio_trigger_on(this->dev_, this->port_);
  if (err) {
    LOG_ERR("unable to activate port#%d: %d", this->port_, err);
    return Trigger::ErrorCode::Internal;
  }

  k_work_schedule(&this->dwork_, K_MSEC(duration));

  this->duration_ = duration;
  TriggerMsg msg = {
    .Port = this->port_,
    .On = true,
  };
  zbus_chan_pub(&trigger_chan, &msg, K_MSEC(CONFIG_APP_TRIGGER_CHAN_PUB_TIMEOUT));
  return Trigger::ErrorCode::None;
}

Trigger::ErrorCode Trigger::Port::Off()
{
  if (atomic_set(&this->state_, kStateIdle) == kStateIdle) {
    return Trigger::ErrorCode::None;
  }

  int err = gpio_trigger_off(this->dev_, this->port_);
  if (err) {
    LOG_ERR("unable to deactivate port#%d: %d", this->port_, err);
    return Trigger::ErrorCode::Internal;
  }

  this->duration_ = 0;
  TriggerMsg msg = {
    .Port = this->port_,
    .On = false,
  };
  zbus_chan_pub(&trigger_chan, &msg, K_MSEC(CONFIG_APP_TRIGGER_CHAN_PUB_TIMEOUT));

  return ErrorCode::None;
}
