#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>
#include <span>

#include "bytes.hpp"
#include "defs.hpp"
#include "mac.hpp"
#include "result.hpp"

namespace toad {

struct EthernetFrame {
  MAC dst;
  MAC src;
  u16 ethertype;

  std::vector<u8> _payload;

  EthernetFrame() {}

  EthernetFrame(MAC dst, MAC src, u16 ethertype, u8 *ptr, sz size)
      : dst(dst), src(src), ethertype(ethertype), _payload(ptr, ptr + size) {}

  static const sz DST_SRC_ETHETYPE_SZ = 6 + 6 + 2;

  static auto from_bytes(Bytes bytes) -> Result<EthernetFrame, sz> {
    if (bytes.size < DST_SRC_ETHETYPE_SZ)
      return bytes.size;

    auto frame = EthernetFrame();

    bytes.read_array(frame.dst.data(), 6);
    bytes.read_array(frame.src.data(), 6);

    frame.ethertype = *bytes.read_u16();

    sz size = bytes.remaining();
    frame._payload = std::vector<u8>(size);
    bytes.read_rest(frame._payload.data());
    spdlog::info("Frame Payload Size: {}", size);

    return frame;
  }

  auto payload() -> Bytes { return Bytes(_payload.data(), _payload.size()); }

  sz buffer_size() const { return _payload.size() + DST_SRC_ETHETYPE_SZ; }

  void to_bytes(Bytes bytes) const {
    ASSERT(bytes.size > buffer_size(),
           "The buffer must fit the ethernet packet");

    bytes.write_array(dst.data(), 6);
    bytes.write_array(src.data(), 6);
    bytes.write_u16(ethertype);
    bytes.write_rest(_payload.data(), _payload.size());
  }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::EthernetFrame> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::EthernetFrame &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "{} -> {}  ethertype=0x{:04X} payload_size={}", f.src,
                    f.dst, f.ethertype, f._payload.size());
    return out;
  }
};

} // namespace fmt
