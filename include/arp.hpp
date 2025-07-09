#pragma once

#include <array>
#include <optional>
#include <span>

#include <fmt/core.h>
#include <fmt/format.h>

#include "defs.hpp"
#include "result.hpp"

namespace toad {

template <u16 htype, u16 ptype, u8 hlen, u8 plen> struct Arp {
  u16 oper;

  std::array<u8, hlen> sender_hardware_addr, target_hardware_addr;
  std::array<u8, plen> sender_protocol_addr, target_protocol_addr;

  static constexpr sz EXPECTED_LENGTH_BUFFER =
      sizeof(htype) + sizeof(ptype) + sizeof(Arp::oper) + sizeof(hlen) +
      sizeof(plen) + 2 * plen + 2 * hlen;

  Arp(u16 oper, std::array<u8, hlen> sha, std::array<u8, plen> spa,
      std::array<u8, hlen> tha, std::array<u8, plen> tpa)
      : sender_hardware_addr(sha), target_hardware_addr(tha),
        sender_protocol_addr(spa), target_protocol_addr(tpa), oper(oper) {}

  static auto from_buffer(const std::span<u8> buffer) -> Result<Arp, sz> {
    if (buffer.size_bytes() < EXPECTED_LENGTH_BUFFER)
      return 0;
    if (buffer.size_bytes() - EXPECTED_LENGTH_BUFFER)
      return EXPECTED_LENGTH_BUFFER;

    auto *data = buffer.data();
    auto read_ne16 = [&](size_t off) {
      return u16(data[off] << 8 | data[off + 1]);
    };

    if (read_ne16(0) != htype)
      return 0;
    if (read_ne16(2) != ptype)
      return 2;
    if (data[4] != hlen)
      return 4;
    if (data[5] != plen)
      return 5;

    auto oper = read_ne16(6);
    auto arp = Arp(oper, {}, {}, {}, {});

    std::copy_n(data + 8, hlen, arp.sender_hardware_addr.begin());
    std::copy_n(data + 8 + hlen, plen, arp.sender_protocol_addr.begin());
    std::copy_n(data + 8 + hlen + plen, hlen, arp.target_hardware_addr.begin());
    std::copy_n(data + 8 + hlen + plen + hlen, plen,
                arp.target_protocol_addr.begin());

    return arp;
  }

  bool to_buffer(std::span<u8> buffer) {
    if (buffer.size_bytes() < EXPECTED_LENGTH_BUFFER)
      return false;

    auto write_ne16 = [&](size_t off, u16 data) {
      buffer[off] = (data >> 8) & 0xFF;
      buffer[off + 1] = data & 0xFF;
    };

    auto *data = buffer.data();
    write_ne16(0, htype);
    write_ne16(2, ptype);
    buffer[4] = hlen;
    buffer[5] = plen;

    write_ne16(6, oper);

    std::copy_n(sender_hardware_addr.begin(), hlen, data + 8);
    std::copy_n(sender_protocol_addr.begin(), plen, data + 8 + hlen);
    std::copy_n(target_hardware_addr.begin(), hlen, data + 8 + hlen + plen);
    std::copy_n(target_protocol_addr.begin(), plen,
                data + 8 + hlen + plen + hlen);

    return true;
  }
};

constexpr u16 HTYPE_ETHERNET_1 = 0x0001;
constexpr u16 PTYPE_IPV4 = 0x0800;

using ArpIPv4 = Arp<HTYPE_ETHERNET_1, PTYPE_IPV4, 6, 4>;

} // namespace toad

namespace fmt {

template <toad::u16 htype, toad::u16 ptype, toad::u8 hlen, toad::u8 plen>
struct formatter<toad::Arp<htype, ptype, hlen, plen>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::Arp<htype, ptype, hlen, plen> &f,
              FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out,
                    "<ARP htype=0x{:04X} ptype=0x{:04X} hlen={} plen={} op={}>",
                    htype, ptype, hlen, plen, f.oper);
    return out;
  }
};

} // namespace fmt
