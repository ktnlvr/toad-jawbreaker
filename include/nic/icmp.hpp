#pragma once

#include "checksum.hpp"
#include "packet.hpp"
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
  toad::checksum checksum;
  // TODO: maybe use a union instead?
  std::array<u8, 4> rest;
  Buffer payload;

  static constexpr auto header_size() -> sz { return 8; }
  auto header_dynamic_size() -> sz { return 0; }
  auto payload_size() -> sz { return payload.size; }

  auto size() -> sz {
    return header_size() + header_dynamic_size() + payload_size();
  }

  Icmp() {}

  template <TypestateDirection other>
  Icmp(const Icmp<other> &icmp)
      : type(icmp.type), code(icmp.code), checksum(icmp.checksum),
        rest(icmp.rest), payload(icmp.payload) {}

  static Icmp try_from_stream(ByteIStream &stream) {
    Icmp ret;

    stream.read_u8((u8 *)&ret.type)
        .read_u8(&ret.code)
        .read_u16(&ret.checksum.n)
        .read_array(&ret.rest)
        .read_buffer(&ret.payload);

    // NOTE(Artur): If we are sending a malformed packet its a logical error,
    // but if we receive it from the user the checksum mismatch might be
    // malicious and should be handled gracefully.
    // TODO: fail on checksum mistmatch
    if constexpr (direction == DirectionOut)
      ASSERT(ret.checksum == ret.calculate_checksum(),
             "Calculated checksum must match received checksum.");

    return ret;
  }

  void try_to_stream(ByteOStream &bytes) const {
    bytes.write_u8((u8)type)
        .write_u8(code)
        .write_u16(checksum)
        .write_array(rest)
        .write_buffer(payload);
  }

  sz buffer_size() const { return 1 + 1 + 2 + rest.size() + payload.size; }

  toad::checksum calculate_checksum() const {
    // NOTE(Artur): Round up to a multiple of two, required by the spec
    sz padded_buffer_size = (buffer_size() + 1) & ~1;
    Buffer buffer(padded_buffer_size);
    auto ostream = ByteOStream(buffer);
    try_to_stream(ostream);

    // Zero out the place where the checksum would be
    // equivalent to skipping the checksum field
    buffer.data()[2] = 0;
    buffer.data()[3] = 0;

    return toad::checksum({buffer.data(), buffer.size});
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

static_assert(Packet<Icmp<DirectionIn>>);

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