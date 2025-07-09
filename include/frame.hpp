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

  sz payload_size;
  u8 *payload;

  EthernetFrame(MAC dst, MAC src, u16 ethertype, std::span<u8> payload)
      : dst(dst), src(src), ethertype(ethertype),
        payload_size(payload.size_bytes()) {
    u8 *payload_copy = new u8[payload_size];
    memcpy(payload_copy, payload.data(), payload_size);
    this->payload = payload_copy;
  }

  ~EthernetFrame() { delete[] payload; }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::EthernetFrame> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::EthernetFrame &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "{} -> {}  ethertype=0x{:04X} payload_size={}", f.src,
                    f.dst, f.ethertype, f.payload_size);
    return out;
  }
};

} // namespace fmt
