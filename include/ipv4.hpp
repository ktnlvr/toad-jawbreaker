#pragma once

#include <array>
#include <span>

#include "bytes.hpp"
#include "defs.hpp"
#include "typestate.hpp"

namespace toad {

constexpr u16 ETHERTYPE_IPV4 = 0x0800;

struct IPv4 : std::array<u8, 4> {
  using std::array<u8, 4>::array;

  IPv4(const std::array<u8, 4> &arr) : std::array<u8, 4>(arr) {}
};

template <TypestateDirection direction> struct Ip {
  u8 version;
  u8 ihl;
  u8 dscp;
  u8 ecn;
  u16 total_length;
  u16 identification;
  u8 flags;
  u16 fragment_offset;
  u8 ttl;
  u8 protocol;
  u16 header_checksum;
  IPv4 src;
  IPv4 dst;
  /// NOTE(Artur): Options would go here, but I'm not sure how to parse them yet
  std::span<u8> payload;

  Ip() {}

  static Ip try_from_bytes(Bytes &bytes) {
    Ip ret;

    u8 version_ihl = bytes.read_u8();
    ret.version = (version_ihl >> 4) & 0xF;
    ret.ihl = version_ihl & 0xF;

    u8 dscp_ecn = bytes.read_u8();
    ret.dscp = (dscp_ecn >> 2) & 0b111111;
    ret.ecn = dscp_ecn & 0b11;

    ret.total_length = bytes.read_u16();
    ret.identification = bytes.read_u16();

    u16 flags_fragment_offset = bytes.read_u16();
    ret.flags = (flags_fragment_offset >> 13) & 0b111;
    ret.fragment_offset = (flags_fragment_offset & 0x1FFF);

    ret.ttl = bytes.read_u8();
    ret.protocol = bytes.read_u8();
    ret.header_checksum = bytes.read_u16();

    bytes.read_array(ret.src);
    bytes.read_array(ret.dst);
    // TODO: optional field
    sz payload_size = bytes.remaining() - 4;
    ret.payload = {
        bytes.ptr,
        payload_size,
    };

    return ret;
  }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::IPv4> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::IPv4 &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "{}.{}.{}.{}", f[0], f[1], f[2], f[3]);
    return out;
  }
};

template <toad::TypestateDirection direction>
struct formatter<toad::Ip<direction>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::Ip<direction> &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "<IP {} -> {}>", f.src, f.dst);
    return out;
  }
};

} // namespace fmt