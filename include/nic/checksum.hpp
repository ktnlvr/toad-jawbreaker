#pragma once

#include <span>

#include "defs.hpp"

namespace toad {

/// @brief A wrapper for the  the [Internet
/// Checksum](https://en.wikipedia.org/wiki/Internet_checksum) specified
/// in [RFC791](https://www.rfc-editor.org/rfc/rfc791). Allows trivial
/// calculation and addition.
struct checksum {
  u16 n = 0;

  checksum() {}

  checksum(u16 checksum) : n(checksum) {}

  explicit checksum(std::span<u8> span) {
    ASSERT(span.size() % 2 == 0, "The buffer must be a multiple of two.");
    ASSERT(span.size() > 0, "The buffer size must be positive");

    u32 acc = 0;
    for (sz i = 0; i < span.size(); i += 2) {
      // Assumed to be BE
      u16 word = (span[i] << 8) | span[i + 1];
      acc += word;

      // Overflow carried into the LSB bit
      if (acc > 0xFFFF)
        acc = (acc & 0xFFFF) + 1;
    }

    acc = (acc & 0xFFFF) + (acc >> 16);
    n = ~acc & 0xFFFF;
  }

  checksum &operator+=(const checksum &rhs) {
    *this = *this + rhs;
    return *this;
  }

  checksum operator+(const checksum &rhs) const {
    u32 sum = this->n + rhs.n;
    return (sum & 0xFFFF) + (sum >> 16);
  }

  u16 operator*() const { return n; }

  operator u16() const { return n; }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::checksum> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::checksum &f, FormatContext &ctx) {
    auto out = ctx.out();
    out = format_to(out, "{}", f.n);
    return out;
  }
};

} // namespace fmt