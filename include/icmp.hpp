#pragma once

#include "bytes.hpp"
#include "checksum.hpp"
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

  Icmp() {}

  template <TypestateDirection other>
  Icmp(const Icmp<other> &icmp)
      : type(icmp.type), code(icmp.code), checksum(icmp.checksum),
        rest(icmp.rest), payload(icmp.payload) {}

  static Icmp try_from_bytes(Bytes &bytes) {
    Icmp ret;

    ret.type = (IcmpType)bytes.read_u8();
    ret.code = bytes.read_u8();
    ret.checksum = bytes.read_u16();
    bytes.read_array(ret.rest);
    bytes.read_vector(ret.payload);

    // NOTE(Artur): If we are sending a malformed packet its a logical error,
    // but if we receive it from the user the checksum mismatch might be
    // malicious and should be handled gracefully.
    // TODO: fail on checksum mistmatch
    if constexpr (direction == DirectionOut)
      ASSERT(ret.checksum == ret.calculate_checksum(),
             "Calculated checksum must match received checksum.");

    return ret;
  }

  void try_to_bytes(Bytes &bytes) const {
    bytes.write_u8((u8)type);
    bytes.write_u8(code);
    bytes.write_u16(checksum);
    bytes.write_array(rest);
    bytes.write_vector(payload);
  }

  sz buffer_size() const { return 1 + 1 + 2 + rest.size() + payload.size(); }

  u16 calculate_checksum() const {
    // NOTE(Artur): Round up to a multiple of two, required by the spec
    sz padded_buffer_size = (buffer_size() + 1) & ~1;
    std::vector<u8> buffer(padded_buffer_size);

    Bytes bytes(buffer);
    try_to_bytes(bytes);

    // Zero out the place where the checksum would be
    // equivalent to skipping the checksum field
    buffer[2] = 0;
    buffer[3] = 0;

    return internet_checksum(buffer);
  }

  auto clone_as_echo_response() const -> Icmp<~direction> {
    ASSERT(this->type == IcmpType::EchoRequest,
           "Only ICMP Echo Requests can be made into Echo Responses");

    Icmp<~direction> response = Icmp<~direction>(*this);
    spdlog::warn("Swapping from {} to {}", response.type, IcmpType::EchoReply);
    response.type = IcmpType::EchoReply;
    response.checksum = response.calculate_checksum();
    return response;
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