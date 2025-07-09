#pragma once

#include "defs.hpp"

namespace toad {

struct MAC : std::array<u8, 6> {};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::MAC> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::MAC &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", f[0],
                    f[1], f[2], f[3], f[4], f[5]);
    return out;
  }
};

} // namespace fmt
