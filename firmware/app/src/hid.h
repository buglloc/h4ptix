#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>


namespace H4X::HID {
  template <size_t MsgSize, size_t MaxMsgs>
  class MsgRingBuffer {
  public:
    MsgRingBuffer() {
      ring_buf_init(&buf_, (sizeof(uint16_t) + MsgSize) * MaxMsgs, bufMem_);
    }

    int Put(const uint8_t* data, uint16_t len) {
      if (len == 0) {
        return 0;
      }

      uint16_t size = len + sizeof(uint16_t);
      if (size > MsgSize) {
        return -E2BIG;
      }

      uint8_t *tmp;
      if (ring_buf_put_claim(&buf_, &tmp, size) != size) {
        return -ENOMEM;
      }

      std::memcpy(tmp, &len, sizeof(uint16_t));
      std::memcpy(tmp + sizeof(uint16_t), data, len);

      return ring_buf_put_finish(&buf_, size);
    }

    int Put(const std::vector<uint8_t> &data) {
      return this->Put(data.data(), data.size());
    }

    int Get(std::vector<uint8_t> &out) {
      uint16_t len = 0;
      uint32_t got = ring_buf_get(&buf_, reinterpret_cast<uint8_t*>(&len), sizeof(uint16_t));
      if (got == 0) {
        return -ENOMSG;
      }

      if (len > MsgSize - sizeof(len)) {
        return -E2BIG;
      }

      out.resize(len);
      got = ring_buf_get(&buf_, out.data(), len);
      if (got != len) {
        return -EIO;
      }

      return 0;
    }

  private:
    struct ring_buf buf_;
    uint8_t bufMem_[(sizeof(uint16_t) + MsgSize) * MaxMsgs];
  };

  int Init();
  int Recv(std::vector<uint8_t> &out, k_timeout_t timeout);
  int Send(const uint8_t *data, size_t len);
}
