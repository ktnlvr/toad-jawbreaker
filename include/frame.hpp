#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>
#include <span>

#include "bytes.hpp"
#include "defs.hpp"
#include "mac.hpp"
#include "result.hpp"

namespace toad {

struct EthernetFrame {
  MAC dst;
  MAC src;
  u16 ethertype;

  std::vector<u8> _payload;

  EthernetFrame() {}

  EthernetFrame(MAC dst, MAC src, u16 ethertype, u8 *ptr, sz size)
      : dst(dst), src(src), ethertype(ethertype), _payload(ptr, ptr + size) {}

  static const sz MANDATORY_BUFFER_SIZE = 6 + 6 + 2;

  static auto from_bytes(Bytes bytes) -> Result<EthernetFrame, sz> {
    if (bytes.size < MANDATORY_BUFFER_SIZE)
      return bytes.size;

    auto frame = EthernetFrame();

    bytes.read_array(frame.dst.data(), 6);
    bytes.read_array(frame.src.data(), 6);

    frame.ethertype = *bytes.read_u16();

    sz size = bytes.remaining();
    frame._payload = std::vector<u8>(size);
    bytes.read_rest(frame._payload.data());
    spdlog::info("Frame Payload Size: {}", size);

    return frame;
  }

  auto payload() -> Bytes { return Bytes(_payload.data(), _payload.size()); }

  sz to_buffer(std::span<u8> data) {
    sz size = MANDATORY_BUFFER_SIZE + _payload.size();

    ASSERT(data.size_bytes() >= size,
           "Output buffer must fit {} + {} = {}, but it only fits {}",
           MANDATORY_BUFFER_SIZE, _payload.size(), size, data.size_bytes());

    std::memcpy(data.data(), dst.data(), 6);
    std::memcpy(data.data() + 6, src.data(), 6);
    data.data()[12] = ethertype & 0xFF;
    data.data()[13] = (ethertype >> 8) & 0xFF;
    return size;
  }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::EthernetFrame> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::EthernetFrame &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "{} -> {}  ethertype=0x{:04X} payload_size={}", f.src,
                    f.dst, f.ethertype, f._payload.size());
    return out;
  }
};

} // namespace fmt
