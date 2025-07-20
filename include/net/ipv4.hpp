#pragma once

#include <array>
#include <span>
#include <vector>

#include "../bytes/bytestream.hpp"
#include "checksum.hpp"
#include "defs.hpp"
#include "packet.hpp"
#include "typestate.hpp"

namespace toad {

constexpr u16 ETHERTYPE_IPV4 = 0x0800;

struct IPv4 : std::array<u8, 4> {
  using std::array<u8, 4>::array;

  IPv4(const std::array<u8, 4> &arr) : std::array<u8, 4>(arr) {}
};

constexpr sz IP_HEADER_SIZE = 20;

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
  checksum header_checksum;
  IPv4 src;
  IPv4 dst;
  /// NOTE(Artur): Options would go here, but I'm not sure how to parse them yet
  Buffer payload;

  static constexpr auto header_size() -> sz { return 20; }
  auto header_dynamic_size() -> sz {
    // TODO: handle options
    return 0;
  }
  auto payload_size() -> sz { return payload.size; }

  auto size() -> sz {
    return header_size() + header_dynamic_size() + payload_size();
  }

  Ip() {}

  template <TypestateDirection other>
  explicit Ip(const Ip<other> &ip)
      : version(ip.version), ihl(ip.ihl), dscp(ip.dscp), ecn(ip.ecn),
        total_length(ip.total_length), identification(ip.identification),
        flags(ip.flags), fragment_offset(ip.fragment_offset), ttl(ip.ttl),
        protocol(ip.protocol), header_checksum(ip.header_checksum), src(ip.src),
        dst(ip.dst) {}

  static Ip try_from_stream(ByteIStream &stream) {
    Ip ret;

    u8 version_ihl;
    u8 dscp_ecn;
    u16 flags_fragment_offset;

    stream.read_u8(&version_ihl)
        .read_u8(&dscp_ecn)
        .read_u16(&ret.total_length)
        .read_u16(&ret.identification)
        .read_u16(&flags_fragment_offset)
        .read_u8(&ret.ttl)
        .read_u8(&ret.protocol)
        .read_u16(&ret.header_checksum.n)
        .read_array(&ret.src)
        .read_array(&ret.dst)
        .read_buffer(&ret.payload);

    // TODO: return result
    ASSERT(stream.errc == ByteStreamErrorCode::Ok,
           "Failed to read {} bytes, IP packet at cursor={}: {}",
           (int)stream.errc, stream.last_size, stream.cursor, stream.buffer);

    ret.version = (version_ihl >> 4) & 0xF;
    ret.ihl = version_ihl & 0xF;

    ret.dscp = (dscp_ecn >> 2) & 0b111111;
    ret.ecn = dscp_ecn & 0b11;

    ret.flags = (flags_fragment_offset >> 13) & 0b111;
    ret.fragment_offset = (flags_fragment_offset & 0x1FFF);

    return ret;
  }

  void try_to_stream(ByteOStream &stream) const {
    stream.write_u8((version << 4) | ihl)
        .write_u8((dscp << 2) | ecn)
        .write_u16(total_length)
        .write_u16(identification);

    // NOTE(Artur): Some additional bytes
    // swizzling needs to be done since
    // this is technically a 2-byte word
    u8 hi_byte = (flags << 5) | ((fragment_offset >> 8) & 0x1F);
    u8 lo_byte = fragment_offset & 0xFF;
    stream.write_u8(hi_byte).write_u8(lo_byte);

    stream.write_u8(ttl)
        .write_u8(protocol)
        .write_u16(header_checksum)
        .write_array(src)
        .write_array(dst)
        .write_buffer(payload);
  }

  auto clone_as_response(Buffer payload, u8 ttl = 64) const -> Ip<~direction> {
    auto response = Ip<~direction>(*this);

    response.payload = payload;
    response.total_length = buffer_size();
    response.ttl = ttl;
    std::swap(response.dst, response.src);
    response.header_checksum = response.calculate_checksum();
    return response;
  }

  auto buffer_size() const -> sz {
    // TODO(Artur): respect optional data
    return IP_HEADER_SIZE + payload.size;
  }

  checksum calculate_checksum() const {
    // NOTE(Artur): Round up to a multiple of two, required by the spec
    sz padded_buffer_size = (buffer_size() + 1) & ~1;
    Buffer buffer(padded_buffer_size);
    auto ostream = ByteOStream(buffer);
    try_to_stream(ostream);

    // Zero out the place where the checksum would be
    // equivalent to skipping the checksum field
    buffer.data()[10] = 0;
    buffer.data()[11] = 0;

    return checksum({buffer.data(), buffer.size});
  }
};

static_assert(Packet<Ip<DirectionIn>>);

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
    out = format_to(out, "<IP {} -> {} protocol=0x{:02X}>", f.src, f.dst,
                    f.protocol);
    return out;
  }
};

} // namespace fmt