#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>
#include <stdint.h>

#define ASSERT(expr, msg, ...)                                                 \
  do {                                                                         \
    if (!(expr)) {                                                             \
      spdlog::critical("Assertion `" #expr "` failed! {}:{}\t" msg, __FILE__,  \
                       __LINE__, ##__VA_ARGS__);                               \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

namespace toad {

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using sz = size_t;
using ssz = ssize_t;

} // namespace toad

namespace fmt {

template <typename T> struct formatter<std::vector<T>> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const std::vector<T> &v, FormatContext &ctx) const {
    auto out = ctx.out();
    format_to(out, "[");
    if (v.size() > 0) {
      format_to(out, "{}", v[0]);
      for (toad::sz i = 1; i < v.size(); i++)
        format_to(out, " {}", v[i]);
    }
    format_to(out, "]");
    return out;
  }
};

} // namespace fmt
