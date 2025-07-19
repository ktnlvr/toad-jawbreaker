#pragma once

#include <array>
#include <optional>
#include <span>

#include <fmt/core.h>
#include <fmt/format.h>

#include "bytes.hpp"
#include "defs.hpp"
#include "ipv4.hpp"
#include "mac.hpp"

namespace toad {

constexpr u16 ETHERTYPE_ARP = 0x0806;

template <u16 htype, u16 ptype, u8 hlen, u8 plen> struct Arp {
  u16 oper;

  std::array<u8, hlen> sender_hardware_addr, target_hardware_addr;
  std::array<u8, plen> sender_protocol_addr, target_protocol_addr;

  static constexpr sz EXPECTED_BUFFER_LENGTH =
      sizeof(htype) + sizeof(ptype) + sizeof(Arp::oper) + sizeof(hlen) +
      sizeof(plen) + 2 * plen + 2 * hlen;

  Arp(u16 oper, std::array<u8, hlen> sha, std::array<u8, plen> spa,
      std::array<u8, hlen> tha, std::array<u8, plen> tpa)
      : sender_hardware_addr(sha), target_hardware_addr(tha),
        sender_protocol_addr(spa), target_protocol_addr(tpa), oper(oper) {}

  static auto try_from_bytes(Bytes &bytes) -> Arp {
    u16 parsed_htype = bytes.read_u16();
    u16 parsed_ptype = bytes.read_u16();
    u8 parsed_hlen = bytes.read_u8();
    u8 parsed_plen = bytes.read_u8();
    u16 oper = bytes.read_u16();

    std::array<u8, hlen> sha, tha;
    std::array<u8, plen> spa, tpa;

    bytes.read_array(sha);
    bytes.read_array(spa);
    bytes.read_array(tha);
    bytes.read_array(tpa);

    return Arp(oper, sha, spa, tha, tpa);
  }

  void try_to_bytes(Bytes &bytes) {
    bytes.write_u16(htype);
    bytes.write_u16(ptype);
    bytes.write_u8(hlen);
    bytes.write_u8(plen);
    bytes.write_u16(oper);
    bytes.write_array(sender_hardware_addr);
    bytes.write_array(sender_protocol_addr);
    bytes.write_array(target_hardware_addr);
    bytes.write_array(target_protocol_addr);
  }

  auto clone_as_response(std::array<u8, hlen> respond_with_hardware_addr) const
      -> Arp {

    Arp ret = *this;
    ret.oper = 0x0002;
    ret.target_hardware_addr = sender_hardware_addr;
    ret.target_protocol_addr = sender_protocol_addr;
    ret.sender_protocol_addr = target_protocol_addr;
    ret.sender_hardware_addr = respond_with_hardware_addr;

    return ret;
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

template <> struct formatter<toad::ArpIPv4> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::ArpIPv4 &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out,
                    "<ARP IPv4 op={} "
                    "sha={} spa={} tha={} tpa={}>",
                    f.oper, toad::MAC(f.sender_hardware_addr),
                    toad::IPv4(f.sender_protocol_addr),
                    toad::MAC(f.target_hardware_addr),
                    toad::IPv4(f.target_protocol_addr));
    return out;
  }
};

} // namespace fmt
