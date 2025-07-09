#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>
#include <span>

#include "defs.hpp"

namespace toad {

struct MAC : std::array<u8, 6> {};

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
