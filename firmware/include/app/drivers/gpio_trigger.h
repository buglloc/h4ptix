#ifndef HM_DRIVER_GPIO_TRIGGER_H_
#define HM_DRIVER_GPIO_TRIGGER_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*gpio_trigger_api_off)(const struct device *dev, uint32_t port);
typedef int (*gpio_trigger_api_set)(const struct device *dev, uint32_t port, bool active);
typedef int (*gpio_trigger_api_on)(const struct device *dev, uint32_t port);
typedef size_t (*gpio_trigger_api_size)(const struct device *dev);

__subsystem struct trigger_driver_api {
  gpio_trigger_api_set set;
  gpio_trigger_api_on on;
  gpio_trigger_api_off off;
  gpio_trigger_api_size size;
};

__syscall int gpio_trigger_set(const struct device *dev, uint32_t port, bool active);

static inline int z_impl_gpio_trigger_set(const struct device *dev, uint32_t port, bool active)
{
  const struct trigger_driver_api *api = (const struct trigger_driver_api *)dev->api;

  if (api->set == NULL) {
    return -ENOSYS;
  }
  return api->set(dev, port, active);
}

__syscall int gpio_trigger_on(const struct device *dev, uint32_t port);

static inline int z_impl_gpio_trigger_on(const struct device *dev, uint32_t port)
{
  const struct trigger_driver_api *api = (const struct trigger_driver_api *)dev->api;
  if (api->on == NULL) {
    return -ENOSYS;
  }

  return api->on(dev, port);
}

__syscall int gpio_trigger_off(const struct device *dev, uint32_t port);

static inline int z_impl_gpio_trigger_off(const struct device *dev, uint32_t port)
{
  const struct trigger_driver_api *api = (const struct trigger_driver_api *)dev->api;
  if (api->off == NULL) {
    return -ENOSYS;
  }

  return api->off(dev, port);
}

struct trigger_dt_spec {
  const struct device *dev;
  uint32_t index;
};

__syscall uint32_t gpio_trigger_size(const struct device *dev);

static inline uint32_t z_impl_gpio_trigger_size(const struct device *dev)
{
  const struct trigger_driver_api *api = (const struct trigger_driver_api *)dev->api;

  if (api->size == NULL) {
    return -ENOSYS;
  }
  return api->size(dev);
}

static inline int gpio_trigger_set_dt(const struct trigger_dt_spec *spec, bool active)
{
  return gpio_trigger_set(spec->dev, spec->index, active);
}

static inline int gpio_trigger_on_dt(const struct trigger_dt_spec *spec)
{
  return gpio_trigger_on(spec->dev, spec->index);
}

static inline int gpio_trigger_off_dt(const struct trigger_dt_spec *spec)
{
  return gpio_trigger_off(spec->dev, spec->index);
}

static inline size_t gpio_trigger_size_dt(const struct trigger_dt_spec *spec)
{
  return gpio_trigger_size(spec->dev);
}

static inline bool gpio_trigger_is_ready_dt(const struct trigger_dt_spec *spec)
{
  return device_is_ready(spec->dev);
}

#define TRIGGER_DT_SPEC_GET(node_id)                              \
  {                                                               \
    .dev = DEVICE_DT_GET(DT_PARENT(node_id)),                     \
    .index = DT_NODE_CHILD_IDX(node_id),                          \
  }

#define TRIGGER_DT_SPEC_GET_OR(node_id, default_value)            \
  COND_CODE_1(DT_NODE_EXISTS(node_id),                            \
    (LED_DT_SPEC_GET(node_id)),                                   \
    (default_value))

#ifdef __cplusplus
}
#endif

#include <syscalls/gpio_trigger.h>

#endif /* HM_DRIVER_GPIO_TRIGGER_H_ */
