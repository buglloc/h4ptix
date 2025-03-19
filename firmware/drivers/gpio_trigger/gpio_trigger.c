#define DT_DRV_COMPAT gpio_trigger
#include <zephyr/device.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app/drivers/gpio_trigger.h>

LOG_MODULE_REGISTER(trigger_gpio, CONFIG_GPIO_TRIGGER_LOG_LEVEL);

struct trigger_gpio_config {
  uint32_t num_triggers;
  const struct gpio_dt_spec *trigger;
};

static int trigger_gpio_set(const struct device *dev, uint32_t port, bool active) {
  const struct trigger_gpio_config *config = dev->config;
  const struct gpio_dt_spec *trigger_gpio;

  if (port >= config->num_triggers) {
    return -EINVAL;
  }

  trigger_gpio = &config->trigger[port];

  return gpio_pin_set_dt(trigger_gpio, active ? 1 : 0);
}

static int trigger_gpio_on(const struct device *dev, uint32_t port) {
  return trigger_gpio_set(dev, port, true);
}

static int trigger_gpio_off(const struct device *dev, uint32_t port) {
  return trigger_gpio_set(dev, port, false);
}


uint32_t trigger_gpio_size(const struct device *dev)
{
  const struct trigger_gpio_config *config = dev->config;
  return config->num_triggers;
}

static int trigger_gpio_init(const struct device *dev) {
  const struct trigger_gpio_config *config = dev->config;
  int err = 0;

  if (!config->num_triggers) {
    LOG_ERR("%s: no trigger found (DT child nodes missing)", dev->name);
    err = -ENODEV;
  }

  for (size_t i = 0; (i < config->num_triggers) && !err; i++) {
    const struct gpio_dt_spec *trigger = &config->trigger[i];

    if (!device_is_ready(trigger->port)) {
      LOG_ERR("%s: GPIO device not ready", dev->name);
      return -ENODEV;
    }

    err = gpio_pin_configure_dt(trigger, GPIO_OUTPUT_INACTIVE);
    if (err) {
      LOG_ERR("Cannot configure GPIO (err %d)", err);
      return err;
    }
  }

  return 0;
}

static DEVICE_API(trigger, trigger_gpio_api) = {
  .set = trigger_gpio_set,
  .on = trigger_gpio_on,
  .off = trigger_gpio_off,
  .size = trigger_gpio_size,
};

#define TRIGGER_GPIO_DEVICE(i)                                        \
                                                                      \
  static const struct gpio_dt_spec gpio_dt_spec_##i[] = {             \
    DT_INST_FOREACH_CHILD_SEP_VARGS(i, GPIO_DT_SPEC_GET, (,), gpios)  \
  };                                                                  \
                                                                      \
  static const struct trigger_gpio_config trigger_gpio_config_##i = { \
    .num_triggers = ARRAY_SIZE(gpio_dt_spec_##i),                     \
    .trigger = gpio_dt_spec_##i,                                      \
  };                                                                  \
                                                                      \
  DEVICE_DT_INST_DEFINE(i, trigger_gpio_init, NULL,                   \
          NULL, &trigger_gpio_config_##i,                             \
          POST_KERNEL, CONFIG_GPIO_TRIGGER_INIT_PRIORITY,             \
          &trigger_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(TRIGGER_GPIO_DEVICE)
