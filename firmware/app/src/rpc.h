#pragma once

#include <zephyr/kernel.h>

#include "rpc.pb.h"


namespace H4X::RPC {
  int Init();
  int Accept(rpcpb_Req &out, k_timeout_t timeout);
  int Send(const rpcpb_Rsp &rsp);

  void PutError(rpcpb_Rsp &rsp, rpcpb_ErrCode code);
}
