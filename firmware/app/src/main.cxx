#include <cstddef>
#include <string>
#include <vector>
#include <functional>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <app/vendor/json.h>

#include <app/lib/uarti.h>

#include <app_version.h>
#include "ui.h"
#include "trigger.h"
#include "messages.h"

LOG_MODULE_REGISTER(app);

ZBUS_CHAN_DEFINE(trigger_chan,
  TriggerMsg,
  NULL,
  NULL,
  ZBUS_OBSERVERS(ui_trigger_subscriber),
  ZBUS_MSG_INIT(.Port = 0, .On = false)
);

namespace {
  constexpr std::string_view kDefaultErrMsg = "ship happens";
  constexpr std::string_view kErrKind = "err";

  constexpr uint8_t kErrCodeNotSupported  = 0x01;
  constexpr uint8_t kErrCodeInternal              = 0x02;
  constexpr uint8_t kErrCodeInvalidReq         = 0x03;
  constexpr uint8_t kErrCodePortInvalid        = 0x04;
  constexpr uint8_t kErrCodePortBusy            = 0x05;

  void sendJson(const JsonObjectConst& msg)
  {
    std::string data;
    serializeJson(msg, data);

    int err = uarti_push(data.data(), data.size());
    if (err) {
      LOG_ERR("unable to send response: %d", err);
    }
  }

  void buildError(uint8_t code, std::string msg, JsonObject& dst)
  {
    if (msg.empty()) {
      msg = kDefaultErrMsg;
    }

    dst["kind"] = kErrKind;
    JsonObject body = dst["body"].to<JsonObject>();
    body["code"] = code;
    body["msg"] = msg;
  }

  void sendError(uint8_t code, std::string msg)
  {
    LOG_WRN("unable to process request[%d]: %s", code, msg.c_str());

    JsonDocument rsp;
    JsonObject rspObj = rsp.to<JsonObject>();
    buildError(code, msg, rspObj);
    sendJson(rsp.as<JsonObjectConst>());
  }
}

namespace {
  using HandleFn = std::function<void(const JsonObjectConst& req, JsonObject& rsp)>;
  struct Handler
  {
    std::string Kind;
    HandleFn Fn;
  };

  void handle(const char* reqBuf)
  {
    static JsonDocument req{};
    static JsonDocument rsp{};
    static const std::vector<Handler> handlers = {
      {
        .Kind = "trg",
        .Fn = [](const JsonObjectConst& req, JsonObject& rsp) -> void
        {
          size_t port = req["port"].as<size_t>();
          if (!port) {
            buildError(kErrCodeInvalidReq, "unexpected port: must be >0", rsp);
            return;
          }

          --port;
          H4X::Trigger::ErrorCode err = H4X::Trigger::Trigger(
            port,
            req["duration"].as<size_t>(),
            req["delay"].as<size_t>()
          );
          switch (err) {
          case H4X::Trigger::ErrorCode::None:
            break;
          case H4X::Trigger::ErrorCode::PortInvalid:
            buildError(kErrCodePortInvalid, "requested invalid port", rsp);
            break;
          case H4X::Trigger::ErrorCode::PortBusy:
            buildError(kErrCodePortBusy, "port busy", rsp);
            break;
          default:
            buildError(kErrCodeInternal, "ship happens", rsp);
            break;
          }
        },
      },
    };

    req.clear();
    DeserializationError jsonErr = deserializeJson(req, reqBuf);
    if (jsonErr) {
      if (jsonErr != DeserializationError::Code::EmptyInput) {
        sendError(kErrCodeInvalidReq, jsonErr.c_str());
      }
      return;
    }

    std::string_view kind = req["kind"].as<std::string_view>();
    if (kind.empty()) {
      sendError(kErrCodeInvalidReq, "invalid kind: required");
      return;
    }

    for (const auto& handler : handlers) {
      if (handler.Kind != kind) {
        continue;
      }

      rsp.clear();
      JsonObject rspObj = rsp.to<JsonObject>();
      handler.Fn(req["body"].as<JsonObjectConst>(), rspObj);

      if (rspObj.size() == 0) {
        rspObj["kind"] = "ack";
      }

      sendJson(rsp.as<JsonObjectConst>());
      return;
    }

    sendError(kErrCodeInvalidReq, "command not found");
  }
}

int main(void)
{
  LOG_INF("starting, v%s", APP_VERSION_STRING);

  int err = uarti_init();
  if (err) {
    LOG_ERR("unable to initialize uarti: %d", err);
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

  char reqBuf[CONFIG_UARTI_RX_BUFFER_SIZE*2] __aligned(4) = {};
  while (1) {
    err = uarti_pop(&reqBuf, K_FOREVER);
    if (err) {
      LOG_ERR("unable to pop message from uarti: %d", err);
      k_sleep(K_MSEC(1));
      continue;
    }

    handle(reqBuf);
  }

  return 0;
}
