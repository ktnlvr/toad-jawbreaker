#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <fmt/format.h>
#include <span>

#include "../bytes/bytestream.hpp"
#include "defs.hpp"
#include "mac.hpp"
#include "packet.hpp"

#include "typestate.hpp"

namespace toad {

template <TypestateDirection direction> struct EthernetFrame {
  MAC dst;
  MAC src;
  u16 ethertype;

  Buffer payload;

  static constexpr auto header_size() -> sz { return 14; }
  auto header_dynamic_size() -> sz { return 0; }
  auto payload_size() -> sz { return payload._size; }

  auto size() -> sz {
    return header_size() + header_dynamic_size() + payload_size();
  }

  EthernetFrame() {}

  EthernetFrame(MAC dst, MAC src, u16 ethertype)
      : dst(dst), src(src), ethertype(ethertype) {}

  static const sz DST_SRC_ETHETYPE_SZ = 6 + 6 + 2;

  template <ByteBuffer B>
  static auto try_from_stream(ByteIStream<B> &bytes) -> EthernetFrame {
    EthernetFrame frame;

    bytes.read_array(&frame.dst)
        .read_array(&frame.src)
        .read_u16(&frame.ethertype)
        .read_buffer(&frame.payload);

    // TODO: error check

    return frame;
  }

  auto min_buffer_size() -> sz const {
    return 2 * sizeof(MAC) + 2 + payload._size;
  }

  template <ByteBuffer B> void try_to_stream(ByteOStream<B> &stream) {
    stream.write_array(dst).write_array(src).write_u16(ethertype).write_buffer(
        payload);
  }

  auto clone_as_response(u16 ethertype, sz payload_size,
                         MAC override_sender = MAC())
      -> EthernetFrame<~direction> {
    EthernetFrame<~direction> ret;
    ret.dst = this->src;
    if (!override_sender.is_default())
      ret.src = override_sender;
    ret.ethertype = ethertype;
    ret.payload = Buffer(payload_size);
    return ret;
  }
};

static_assert(Packet<EthernetFrame<DirectionIn>, Buffer>);

} // namespace toad

namespace fmt {

template <toad::TypestateDirection direction>
struct formatter<toad::EthernetFrame<direction>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::EthernetFrame<direction> &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "<Eth {} -> {} ethertype=0x{:04X} payload_size={}>",
                    f.src, f.dst, f.ethertype, f.payload.size);
    return out;
  }
};

} // namespace fmt
