#pragma once

#include <array>
#include <optional>
#include <span>

#include <fmt/core.h>
#include <fmt/format.h>

#include "defs.hpp"
#include "ipv4.hpp"
#include "mac.hpp"
#include "typestate.hpp"

namespace toad {

constexpr u16 ETHERTYPE_ARP = 0x0806;

template <u16 htype, u16 ptype, u8 hlen, u8 plen, TypestateDirection direction>
struct Arp {
  u16 oper;

  std::array<u8, hlen> sender_hardware_addr, target_hardware_addr;
  std::array<u8, plen> sender_protocol_addr, target_protocol_addr;

  static constexpr sz EXPECTED_BUFFER_LENGTH =
      sizeof(htype) + sizeof(ptype) + sizeof(Arp::oper) + sizeof(hlen) +
      sizeof(plen) + 2 * plen + 2 * hlen;

  Arp() {}

  Arp(u16 oper, std::array<u8, hlen> sha, std::array<u8, plen> spa,
      std::array<u8, hlen> tha, std::array<u8, plen> tpa)
      : sender_hardware_addr(sha), target_hardware_addr(tha),
        sender_protocol_addr(spa), target_protocol_addr(tpa), oper(oper) {}

  static auto try_from_stream(ByteIStream &bytes) -> Arp {
    Arp ret;

    u16 parsed_htype, parsed_ptype;
    u8 parsed_hlen, parsed_plen;

    // TODO: error check

    bytes.read_u16(&parsed_htype)
        .read_u16(&parsed_ptype)
        .read_u8(&parsed_hlen)
        .read_u8(&parsed_plen)
        .read_u16(&ret.oper);

    bytes.read_array(&ret.sender_hardware_addr)
        .read_array(&ret.sender_protocol_addr)
        .read_array(&ret.target_hardware_addr)
        .read_array(&ret.target_protocol_addr);

    ASSERT(bytes.errc == ByteStreamErrorCode::Ok,
           "Must have enough bytes to parse ARP");

    return ret;
  }

  void try_to_stream(ByteOStream &bytes) {
    bytes.write_u16(htype)
        .write_u16(ptype)
        .write_u8(hlen)
        .write_u8(plen)
        .write_u16(oper)
        .write_array(sender_hardware_addr)
        .write_array(sender_protocol_addr)
        .write_array(target_hardware_addr)
        .write_array(target_protocol_addr);
  }

  auto copy_as_response(std::array<u8, hlen> respond_with_hardware_addr) const
      -> Arp<htype, ptype, hlen, plen, ~direction> {
    u16 oper = 0x0002;
    std::array<u8, hlen> tha = sender_hardware_addr;
    std::array<u8, plen> tpa = sender_protocol_addr;
    std::array<u8, hlen> sha = respond_with_hardware_addr;
    std::array<u8, plen> spa = target_protocol_addr;

    return Arp<htype, ptype, hlen, plen, ~direction>(oper, sha, spa, tha, tpa);
  }
};

constexpr u16 HTYPE_ETHERNET_1 = 0x0001;
constexpr u16 PTYPE_IPV4 = 0x0800;

template <TypestateDirection direction>
using ArpIPv4 = Arp<HTYPE_ETHERNET_1, PTYPE_IPV4, 6, 4, direction>;

} // namespace toad

namespace fmt {

template <toad::u16 htype, toad::u16 ptype, toad::u8 hlen, toad::u8 plen,
          toad::TypestateDirection direction>
struct formatter<toad::Arp<htype, ptype, hlen, plen, direction>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::Arp<htype, ptype, hlen, plen, direction> &f,
              FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out,
                    "<ARP htype=0x{:04X} ptype=0x{:04X} hlen={} plen={} op={}>",
                    htype, ptype, hlen, plen, f.oper);
    return out;
  }
};

template <toad::TypestateDirection direction>
struct formatter<toad::ArpIPv4<direction>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::ArpIPv4<direction> &f, FormatContext &ctx) {
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
