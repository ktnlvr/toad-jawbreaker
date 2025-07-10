#pragma once

#include <array>
#include <optional>
#include <span>

#include <fmt/core.h>
#include <fmt/format.h>

#include "bytes.hpp"
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

  static auto from_bytes(Bytes bytes) -> Result<Arp, sz> {
    if (bytes.size < EXPECTED_LENGTH_BUFFER)
      return EXPECTED_LENGTH_BUFFER;

    u16 value = *bytes.read_u16();
    ASSERT(value == htype, "Mismatched htype, expected {:04X}, got {:04X}",
           htype, value);
    value = *bytes.read_u16();
    ASSERT(value == ptype, "Mismatched ptype, expected {:04X}, got {:04X}",
           ptype, value);
    value = *bytes.read_u8();
    ASSERT(value == hlen, "Mismatched hlen, expected {:02X}, got {:02X}", hlen,
           value);
    value = *bytes.read_u8();
    ASSERT(value == plen, "Mismatched plen, expected {:02X}, got {:02X}", plen,
           value);

    auto oper = *bytes.read_u16();
    auto arp = Arp(oper, {}, {}, {}, {});

    bytes.read_array(arp.sender_hardware_addr.data(), hlen);
    bytes.read_array(arp.sender_protocol_addr.data(), plen);
    bytes.read_array(arp.target_hardware_addr.data(), hlen);
    bytes.read_array(arp.target_protocol_addr.data(), plen);

    return arp;
  }

  auto to_response(std::array<u8, hlen> respond_with_hardware_addr) const
      -> Arp {

    Arp ret = *this;
    ret.oper = 0x0002;
    ret.target_hardware_addr = sender_hardware_addr;
    ret.target_protocol_addr = sender_protocol_addr;
    ret.sender_protocol_addr = target_protocol_addr;
    ret.sender_hardware_addr = respond_with_hardware_addr;

    return ret;
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
