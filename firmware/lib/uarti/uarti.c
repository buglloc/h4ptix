#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

#include <app/lib/uarti.h>

LOG_MODULE_REGISTER(uarti, CONFIG_UARTI_LOG_LEVEL);

struct buf {
  /* Buffer to hold received data */
  uint8_t *buf;
  /* Number of bytes written to buf */
  size_t len;
  /** Maximum number of bytes in buf */
  size_t len_max;
};

static uint8_t rx_buffer_[CONFIG_UARTI_RX_BUFFER_SIZE] __aligned(4);

static struct buf rx_ = {
  .buf = rx_buffer_,
  .len = 0,
  .len_max = CONFIG_UARTI_RX_BUFFER_SIZE,
};
K_MSGQ_DEFINE(rx_msgq_, CONFIG_UARTI_RX_BUFFER_SIZE, CONFIG_UARTI_RX_QUEUE_SIZE, 4);

static uint8_t tx_buffer_[CONFIG_UARTI_TX_BUFFER_SIZE] __aligned(4);
struct ring_buf tx_ringbuf_;
atomic_t tx_busy_;

static const struct device *uart_dev_;

static void uart_rx_handle(const struct device *dev)
{
  static bool overrun = false;
  uint8_t c;

  while (uart_fifo_read(dev, &c, sizeof(c))) {
    if (overrun) {
      if (c == '\n') {
        overrun = false;
        rx_.len = 0;
      }

      continue;
    }

    if ((c == '\n' || c == '\r') && rx_.len > 0) {
      /* terminate string */
      rx_.buf[rx_.len] = '\0';

      /* if queue is full, message is silently dropped */
      k_msgq_put(&rx_msgq_, rx_.buf, K_NO_WAIT);

      /* reset the buffer (it was copied to the msgq) */
      rx_.len = 0;
      continue;
    }

    if (rx_.len < rx_.len_max) {
      rx_.buf[rx_.len++] = c;
      continue;
    }

    overrun = true;
    LOG_WRN("overrun, incoming message are too big and will be dropped");
  }
}

static void uart_tx_handle(const struct device *dev)
{
  uint32_t len;
  const uint8_t *data;
  int err;

  len = ring_buf_get_claim(&tx_ringbuf_, (uint8_t **)&data, tx_ringbuf_.size);
  if (!len) {
    uart_irq_tx_disable(dev);
    tx_busy_ = 0;
    return;
  }

  LOG_INF("tx3");
  len = uart_fifo_fill(dev, data, len);
  err = ring_buf_get_finish(&tx_ringbuf_, len);
  if (err) {
    LOG_ERR("can't finish tx ringbuf: %d", err);
  }

}

static void uart_callback(const struct device *dev, void *user_data)
{
  ARG_UNUSED(user_data);

  while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
    if (uart_irq_rx_ready(dev)) {
      uart_rx_handle(dev);
    }

    if (uart_irq_tx_ready(dev)) {
      uart_tx_handle(dev);
    }
  }
}

int uarti_push(const char *data, size_t len)
{
  if (len == 0) {
    return 0;
  }

  if (!device_is_ready(uart_dev_)) {
    return -ENODEV;
  }

  ring_buf_put(&tx_ringbuf_, data, len);
  if (data[len-1] != '\n') {
    ring_buf_put(&tx_ringbuf_, "\r\n", 2);
  }

  if (atomic_set(&tx_busy_, 1) == 0) {
    uart_irq_tx_enable(uart_dev_);
  }

  return 0;
}

int uarti_pop(void *msg, k_timeout_t timeout)
{
  return k_msgq_get(&rx_msgq_, msg, timeout);
}

int uarti_init() {
#if DT_NODE_EXISTS(DT_CHOSEN(utbdk_uarti_dev))
  int err = 0;
  uart_dev_ = DEVICE_DT_GET(DT_CHOSEN(utbdk_uarti_dev));

  if (!device_is_ready(uart_dev_)) {
    LOG_ERR("UART device %s is not ready", uart_dev_->name);
    return -ENODEV;
  }

  err = uart_irq_callback_user_data_set(uart_dev_, uart_callback, NULL);
  if (err) {
    switch (err) {
    case -ENOTSUP:
      LOG_ERR("Interrupt-driven UART API support not enabled");
      break;

    case -ENOSYS:
      LOG_ERR("UART device does not support interrupt-driven API");
      break;

    default:
      LOG_ERR("Error setting UART callback: %d", err);
      break;
    }

    return err;
  }

  ring_buf_init(&tx_ringbuf_, CONFIG_UARTI_TX_BUFFER_SIZE, tx_buffer_);
  tx_busy_ = 0;

  uart_irq_rx_enable(uart_dev_);
  return 0;

#else
  LOG_ERR("UART device 'utbdk,uarti-dev' is not present in the device tree");
  return -ENODEV;

#endif
}

#ifdef CONFIG_UARTI_INITIALIZE_AT_BOOT
static int uarti_sys_init(void)
{
  LOG_INF("initalize at boot");
  return uarti_init();
}
SYS_INIT(uarti_sys_init, POST_KERNEL, CONFIG_UARTI_INIT_PRIORITY);
#endif
