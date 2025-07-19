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

  std::vector<u8> _payload;

  EthernetFrame() {}

  EthernetFrame(MAC dst, MAC src, u16 ethertype, u8 *ptr, sz size)
      : dst(dst), src(src), ethertype(ethertype), _payload(ptr, ptr + size) {}

  static const sz DST_SRC_ETHETYPE_SZ = 6 + 6 + 2;

  sz buffer_size() const { return _payload.size() + DST_SRC_ETHETYPE_SZ; }
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
