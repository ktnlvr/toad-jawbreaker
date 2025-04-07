#pragma once

#include <cstddef>
#include <cstdint>

#define DEBUG_BYTE ((toad::byte)0xCC)
#define DEBUG_U32 ((toad::u32)0xDDCCBBAA)

namespace toad {

using u32 = uint_least32_t;
using u16 = uint_least16_t;
using i32 = int_least32_t;
using i16 = int_least16_t;
using byte = unsigned char;
using sz = size_t;

}  // namespace toad
