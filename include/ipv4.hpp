#pragma once

#include <array>

#include "defs.hpp"

namespace toad {

struct IPv4 : std::array<u8, 4> {
  using std::array<u8, 4>::array;

  IPv4(const std::array<u8, 4> &arr) : std::array<u8, 4>(arr) {}
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

} // namespace fmt