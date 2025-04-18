#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>


#include <app_version.h>
#include "ui.h"
#include "trigger.h"
#include "messages.h"
#include "rpc.h"
#include "rpc.pb.h"


LOG_MODULE_REGISTER(app);

ZBUS_CHAN_DEFINE(trigger_chan,
  TriggerMsg,
  NULL,
  NULL,
  ZBUS_OBSERVERS(ui_trigger_subscriber),
  ZBUS_MSG_INIT(.Port = 0, .On = false)
);

using namespace H4X;

namespace {
  void handleTrigger(const rpcpb_Trigger &req, rpcpb_Rsp &rsp)
  {
    if (req.port == 0) {
      LOG_WRN("unexpected port: must be >0");
      RPC::PutError(rsp, rpcpb_ErrCode_ErrCodeInvalidReq);
      return;
    }

    uint32_t port = req.port - 1;
    Trigger::ErrorCode err = Trigger::Trigger(
      port,
      req.duration_ms,
      req.delay_ms
    );
    switch (err) {
    case Trigger::ErrorCode::None:
      LOG_INF("port %zu triggered: duration_ms=%zu delay_ms=%zu", port, req.duration_ms, req.delay_ms);
      break;
    case Trigger::ErrorCode::PortInvalid:
      RPC::PutError(rsp, rpcpb_ErrCode_ErrCodePortInvalid);
      break;
    case Trigger::ErrorCode::PortBusy:
      RPC::PutError(rsp, rpcpb_ErrCode_ErrCodePortBusy);
      break;
    default:
      RPC::PutError(rsp, rpcpb_ErrCode_ErrCodeInternal);
      break;
    }
  }
}

int main(void)
{
  LOG_INF("starting, v%s", APP_VERSION_STRING);

  int err = RPC::Init();
  if (err) {
    LOG_ERR("unable to initialize RPC: %d", err);
    return 1;
  }

  err = Trigger::Init();
  if (err) {
    LOG_ERR("unable to initialize trigger: %d", err);
    return 1;
  }

  err = UI::Init(Trigger::Size());
  if (err) {
    LOG_ERR("unable to initialize UI: %d", err);
    return 1;
  }

  rpcpb_Req req = {};
  while (1) {
    err = RPC::Accept(req, K_FOREVER);
    if (err) {
      LOG_ERR("unable to receive RPC request: %d", err);
      k_sleep(K_MSEC(1));
      continue;
    }

    rpcpb_Rsp rsp = rpcpb_Rsp_init_zero;

    switch (req.which_payload) {
    case rpcpb_Req_trigger_tag:
      handleTrigger(req.payload.trigger, rsp);
      break;

    default:
      RPC::PutError(rsp, rpcpb_ErrCode_ErrCodeInvalidCommand);
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
    RPC::Send(rsp);
  }

  return 0;
}
