#pragma once

#include "bytes.hpp"
#include "typestate.hpp"

#include <vector>

namespace toad {

constexpr u8 PROTOCOL_ICMP = 0x01;

enum struct IcmpType : u8 {
  EchoReply = 0,
  EchoRequest = 8,
};

template <TypestateDirection direction> struct Icmp {
  IcmpType type;
  u8 code;
  u16 checksum;
  // TODO: maybe use a union instead?
  std::array<u8, 4> rest;
  std::vector<u8> payload;

  static Icmp try_from_bytes(Bytes &bytes) {
    Icmp ret;

    ret.type = (IcmpType)bytes.read_u8();
    ret.code = bytes.read_u8();
    ret.checksum = bytes.read_u16();
    bytes.read_array(ret.rest);
    bytes.read_vector(ret.payload);

    return ret;
  }

  void try_to_bytes(Bytes &bytes) {
    bytes.write_u8((u8)type);
    bytes.write_u8(code);
    bytes.write_u16(checksum);
    bytes.write_array(rest);
    bytes.write_vector(payload);
  }
};

} // namespace toad

namespace fmt {

template <toad::TypestateDirection direction>
struct formatter<toad::Icmp<direction>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::Icmp<direction> &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "<ICMP {}>", f.type);
    return out;
  }
};

template <> struct formatter<toad::IcmpType> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::IcmpType &f, FormatContext &ctx) {
    auto out = ctx.out();
    const char *ptr = nullptr;
    switch (f) {
    case toad::IcmpType::EchoReply:
      ptr = "EchoReply";
      break;
    case toad::IcmpType::EchoRequest:
      ptr = "EchoRequest";
      break;
    }

    out = format_to(out, "{}", ptr);
    return out;
  }
};

} // namespace fmt