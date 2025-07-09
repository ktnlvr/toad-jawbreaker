#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>
#include <stdint.h>

#define ASSERT(expr, msg, ...)                                                 \
  do {                                                                         \
    if (!(expr)) {                                                             \
      spdlog::critical("Assertion " #expr " failed! {}:{}\t" msg, __FILE__,    \
                       __LINE__ __VA_ARGS__);                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

namespace toad {

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using sz = size_t;
using ssz = ssize_t;

} // namespace toad
