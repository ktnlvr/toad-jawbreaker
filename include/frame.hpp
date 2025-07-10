#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>
#include <span>

#include "defs.hpp"
#include "mac.hpp"

namespace toad {

struct EthernetFrame {
  MAC dst;
  MAC src;
  u16 ethertype;

  std::vector<u8> payload;

  EthernetFrame(MAC dst, MAC src, u16 ethertype, u8 *ptr, sz size)
      : dst(dst), src(src), ethertype(ethertype), payload(ptr, ptr + size) {}

  static const sz MANDATORY_BUFFER_SIZE = 6 + 6 + 2;

  sz to_buffer(std::span<u8> data) {
    sz size = MANDATORY_BUFFER_SIZE + payload.size();

    ASSERT(data.size_bytes() >= size,
           "Output buffer must fit {} + {} = {}, but it only fits {}",
           MANDATORY_BUFFER_SIZE, payload.size(), size, data.size_bytes());

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
                    f.dst, f.ethertype, f.payload.size());
    return out;
  }
};

} // namespace fmt
