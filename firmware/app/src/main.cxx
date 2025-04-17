#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <pb_encode.h>
#include <pb_decode.h>

#include <app_version.h>
#include "ui.h"
#include "hid.h"
#include "trigger.h"
#include "messages.h"
#include "rpc.pb.h"


LOG_MODULE_REGISTER(app);

ZBUS_CHAN_DEFINE(trigger_chan,
  TriggerMsg,
  NULL,
  NULL,
  ZBUS_OBSERVERS(ui_trigger_subscriber),
  ZBUS_MSG_INIT(.Port = 0, .On = false)
);

namespace {
  void sendResponse(const rpcpb_Rsp &rsp)
  {
    uint8_t buf[rpcpb_Rsp_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
    if (!pb_encode(&stream, rpcpb_Rsp_fields, &rsp)) {
      LOG_WRN("unable to encode response");
      return;
    }

    int err = H4X::HID::Send(buf, stream.bytes_written);
    if (err) {
      LOG_ERR("unable to send response: %d", err);
    }
  }

  void buildError(rpcpb_ErrCode code, rpcpb_Rsp &rsp)
  {
    rsp.which_payload = rpcpb_Rsp_err_tag;
    rsp.payload.err.code = code;
  }

  void sendError(rpcpb_ErrCode code, rpcpb_Rsp &rsp)
  {
    buildError(code, rsp);
    sendResponse(rsp);
  }

  void handleTrigger(const rpcpb_Trigger &req, rpcpb_Rsp &rsp)
  {
    if (req.port == 0) {
      LOG_WRN("unexpected port: must be >0");
      buildError(rpcpb_ErrCode_ErrCodeInvalidReq, rsp);
      return;
    }

    uint32_t port = req.port - 1;
    H4X::Trigger::ErrorCode err = H4X::Trigger::Trigger(
      port,
      req.duration_ms,
      req.delay_ms
    );
    switch (err) {
    case H4X::Trigger::ErrorCode::None:
      LOG_INF("port %zu triggered: delay_ms=%zu duration_ms=%zu", port, req.duration_ms, req.delay_ms);
      break;
    case H4X::Trigger::ErrorCode::PortInvalid:
      buildError(rpcpb_ErrCode_ErrCodePortInvalid,  rsp);
      break;
    case H4X::Trigger::ErrorCode::PortBusy:
      buildError(rpcpb_ErrCode_ErrCodePortBusy,  rsp);
      break;
    default:
      buildError(rpcpb_ErrCode_ErrCodeInternal,  rsp);
      break;
    }
  }

  void handle(const std::vector<uint8_t> &reqPayload)
  {
    rpcpb_Req req = rpcpb_Req_init_zero;
    rpcpb_Rsp rsp = rpcpb_Rsp_init_zero;

    pb_istream_t stream = pb_istream_from_buffer(reqPayload.data(), reqPayload.size());
    if (!pb_decode(&stream, rpcpb_Req_fields, &req)) {
      LOG_WRN("Unable to decode request payload");
      sendError(rpcpb_ErrCode_ErrCodeInvalidReq, rsp);
      return;
    }

    switch (req.which_payload) {
    case rpcpb_Req_trigger_tag:
      handleTrigger(req.payload.trigger, rsp);
      break;

    default:
      buildError(rpcpb_ErrCode_ErrCodeInvalidCommand, rsp);
      break;
    }

    switch (rsp.which_payload) {
    case 0:
      rsp.which_payload = rpcpb_Rsp_ack_tag;
      break;
    case rpcpb_Rsp_err_tag:
      LOG_WRN("unable to process request: %zu", req.id);
      break;
    }

    rsp.id = req.id;
    sendResponse(rsp);
  }
}

int main(void)
{
  LOG_INF("starting, v%s", APP_VERSION_STRING);

  int err = H4X::HID::Init();
  if (err) {
    LOG_ERR("unable to HID: %d", err);
    return 1;
  }

  err = H4X::Trigger::Init();
  if (err) {
    LOG_ERR("unable to initialize trigger: %d", err);
    return 1;
  }

  err = H4X::UI::Init(H4X::Trigger::Size());
  if (err) {
    LOG_ERR("unable to initialize UI: %d", err);
    return 1;
  }

  std::vector<uint8_t> req = {};
  while (1) {
    err = H4X::HID::Recv(req, K_FOREVER);
    if (err) {
      LOG_ERR("unable to receive message from HID: %d", err);
      k_sleep(K_MSEC(1));
      continue;
    }

    handle(req);
  }

  return 0;
}
