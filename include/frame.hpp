#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>
#include <span>

#include "bytes.hpp"
#include "defs.hpp"
#include "mac.hpp"

namespace toad {

struct EthernetFrame {
  MAC dst;
  MAC src;
  u16 ethertype;

  std::vector<u8> payload;

  EthernetFrame() {}

  EthernetFrame(MAC dst, MAC src, u16 ethertype, sz reserve)
      : dst(dst), src(src), ethertype(ethertype), payload(reserve) {}

  static const sz DST_SRC_ETHETYPE_SZ = 6 + 6 + 2;

  static auto try_from_bytes(Bytes &bytes) -> EthernetFrame {
    EthernetFrame frame;
    bytes.read_array(frame.dst);
    bytes.read_array(frame.src);
    frame.ethertype = bytes.read_u16();
    bytes.read_vector(frame.payload);
    return frame;
  }

  auto min_buffer_size() -> sz const {
    return 2 * sizeof(MAC) + 2 + payload.size();
  }

  void try_to_bytes(Bytes &bytes) {
    bytes.write_array(dst);
    bytes.write_array(src);
    bytes.write_u16(ethertype);
    bytes.write_vector(payload);
  }

  auto clone_as_response(u16 ethertype, std::vector<u8> &&payload,
                         MAC override_sender = MAC()) -> EthernetFrame {
    EthernetFrame ret;
    ret.dst = this->src;
    if (!override_sender.is_default())
      ret.src = override_sender;
    ret.ethertype = ethertype;
    ret.payload = payload;
    return ret;
  }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::EthernetFrame> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::EthernetFrame &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "<Eth {} -> {} ethertype=0x{:04X} payload_size={}>",
                    f.src, f.dst, f.ethertype, f.payload.size());
    return out;
  }
};

} // namespace fmt
