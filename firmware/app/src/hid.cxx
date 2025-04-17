#include "hid.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_DECLARE(app, CONFIG_APP_LOG_LEVEL);

using namespace H4X;

/* USB staff */
namespace {
  const uint8_t kHidReportDesc[] = {
    /* Vendor-defined page */
    HID_USAGE_PAGE(0xFF),
    HID_USAGE(0xCE),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
        /* 8 bits per item */
        HID_REPORT_SIZE(8),
        /* 64 bytes */
        HID_REPORT_COUNT(64),
        /* Data, Var, Abs */
        HID_INPUT(0x02),
        /* Data, Var, Abs */
        HID_OUTPUT(0x02),
    HID_END_COLLECTION
  };

  const uint16_t kUsbVid = 0x1209;
  const uint16_t kUsbPid = 0xF601;

  /* Instantiate device */
  const struct device *dev_ = nullptr;

  /* Instantiate a usbd context */
  USBD_DEVICE_DEFINE(usbd_, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), kUsbVid, kUsbPid);
  USBD_DESC_LANG_DEFINE(usb_lang);
  USBD_DESC_MANUFACTURER_DEFINE(usb_mfr, "@UTBDK");
  USBD_DESC_PRODUCT_DEFINE(usb_product, "H4ptiX");
  USBD_DESC_SERIAL_NUMBER_DEFINE(usb_serial);

  /* Full speed configuration */
  USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
  USBD_CONFIGURATION_DEFINE(fs_cfg, 0, 128, &fs_cfg_desc);
}

/* process staff*/
namespace {
  HID::MsgRingBuffer<CONFIG_APP_HID_MAX_MSG_SIZE, CONFIG_APP_HID_QUEUE_SIZE> rxBuf_;
  K_SEM_DEFINE(rxSem_, 0, CONFIG_APP_HID_QUEUE_SIZE);

  void hidIfaceReadyCb(const struct device *dev, const bool ready)
  {
    LOG_INF("HID device %s interface is %s", dev->name, ready ? "ready" : "not ready");
  }

  int hidGetReportCb(const struct device *dev, const uint8_t type, const uint8_t id, const uint16_t len, uint8_t *const buf)
  {
    LOG_INF("HID get report: Type=%u ID=%u", type, id);
    return 0;
  }

  int hidSetReportCb(const struct device *dev, const uint8_t type, const uint8_t id, const uint16_t len, const uint8_t *const buf)
  {
    LOG_INF("HID set report: Type=%u ID=%u", type, id);
    if (type != HID_REPORT_TYPE_OUTPUT) {
      LOG_WRN("Unsupported report type: %d", type);
      return -ENOTSUP;
    }

    if (len == 0) {
      LOG_WRN("Invalid request len: %d", len);
      return -ENOTSUP;
    }

    int err = rxBuf_.Put(buf, len);
    if (err) {
      LOG_WRN("Unable to put data info buf: %d", err);
      return -EIO;
    }

    k_sem_give(&rxSem_);
    return 0;
  }

  void usbMsgCb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
  {
    LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

    if (msg->type == USBD_MSG_CONFIGURATION) {
      LOG_INF("\tConfiguration value %d", msg->status);
    }

    int err = 0;
    if (usbd_can_detect_vbus(usbd_ctx)) {
      if (msg->type == USBD_MSG_VBUS_READY) {
        err = usbd_enable(usbd_ctx);
        if (err) {
          LOG_ERR("Failed to enable device support: %d", err);
        }
      }

      if (msg->type == USBD_MSG_VBUS_REMOVED) {
        err = usbd_disable(usbd_ctx);
        if (err) {
          LOG_ERR("Failed to disable device support: %d", err);
        }
      }
    }
  }

  struct hid_device_ops hid_ops = {
    .iface_ready = hidIfaceReadyCb,
    .get_report = hidGetReportCb,
    .set_report = hidSetReportCb,
  };
}

int HID::Init()
{
  LOG_INF("initialize HID device");
  dev_ = DEVICE_DT_GET_ONE(zephyr_hid_device);
  if (!device_is_ready(dev_)) {
    LOG_ERR("HID Device is not ready");
    return -EIO;
  }

  int err = hid_device_register(dev_, kHidReportDesc, sizeof(kHidReportDesc), &hid_ops);
  if (err) {
    LOG_ERR("Failed to register HID Device: %d", err);
    return err;
  }

  err = usbd_add_descriptor(&usbd_, &usb_lang);
  if (err) {
    LOG_ERR("Failed to initialize language descriptor: %d", err);
    return err;
  }

  err = usbd_add_descriptor(&usbd_, &usb_mfr);
  if (err) {
    LOG_ERR("Failed to initialize manufacturer descriptor: %d", err);
    return err;
  }

  err = usbd_add_descriptor(&usbd_, &usb_product);
  if (err) {
    LOG_ERR("Failed to initialize product descriptor: %d", err);
    return err;
  }


  err = usbd_add_descriptor(&usbd_, &usb_serial);
  if (err) {
    LOG_ERR("Failed to initialize SN descriptor: %d", err);
    return err;
  }

  err = usbd_add_configuration(&usbd_, USBD_SPEED_FS,&fs_cfg);
  if (err) {
    LOG_ERR("Failed to add Full-Speed configuration: %d", err);
    return err;
  }

  err = usbd_register_all_classes(&usbd_, USBD_SPEED_FS, 1, nullptr);
  if (err) {
    LOG_ERR("Failed to add register classes");
    return err;
  }

  usbd_device_set_code_triple(&usbd_, USBD_SPEED_FS, 0, 0, 0);
  err = usbd_init(&usbd_);
  if (err) {
    LOG_ERR("Failed to initialize usb device: %d", err);
    return err;
  }

  err = usbd_msg_register_cb(&usbd_, usbMsgCb);
  if (err) {
    LOG_ERR("Failed to register usb message callback: %d", err);
    return err;
  }

  if (!usbd_can_detect_vbus(&usbd_)) {
    err = usbd_enable(&usbd_);
    if (err) {
      LOG_ERR("Failed to enable usb device support: %d", err);
      return err;
    }
  }

  return 0;
}

int HID::Recv(std::vector<uint8_t> &out, k_timeout_t timeout)
{
  int err = k_sem_take(&rxSem_, timeout);
  if (err) {
    LOG_WRN("Failed to take rx semaphore: %d", err);
    return err;
  }

  return rxBuf_.Get(out);
}

int HID::Send(const uint8_t *data, size_t len)
{
  return hid_device_submit_report(dev_, len, data);
}
