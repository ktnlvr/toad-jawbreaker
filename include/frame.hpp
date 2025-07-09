#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>

#include "defs.hpp"

namespace toad {

struct EthernetFrame {
  u8 dst[6];
  u8 src[6];
  u16 ethertype;

  sz payload_size;
  u8 *payload;
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::EthernetFrame> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::EthernetFrame &f, FormatContext &ctx) {
    auto mac_to_string = [&](auto out, const uint8_t mac[6]) {
      return format_to(out, "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mac[0],
                       mac[1], mac[2], mac[3], mac[4], mac[5]);
    };

    auto out = ctx.out();
    out = mac_to_string(out, f.dst);
    out = format_to(out, " -> ");
    out = mac_to_string(out, f.src);
    out = format_to(out,
                    "   ethertype=0x{:04X}"
                    "   payload_size={}",
                    f.ethertype, f.payload_size);
    return out;
  }
};

} // namespace fmt
