#pragma once

#include <span>

#include "defs.hpp"

namespace toad {

/// @brief Calculates the [Internet
/// Checksum](https://en.wikipedia.org/wiki/Internet_checksum) specified
/// in [RFC791](https://www.rfc-editor.org/rfc/rfc791).
u16 internet_checksum(std::span<u8> buffer) {
  ASSERT(buffer.size() % 2 == 0, "The buffer must be a multiple of two.");
  ASSERT(buffer.size() > 0, "The buffer size must be positive");

  u32 acc = 0;
  for (sz i = 0; i < buffer.size(); i += 2) {
    // Assumed to be BE
    u16 word = (buffer[i] << 8) | buffer[i + 1];
    acc += word;

    // Overflow carried into the LSB bit
    if (acc > 0xFFFF)
      acc = (acc & 0xFFFF) + 1;
  }

  acc = (acc & 0xFFFF) + (acc >> 16);
  return (u16)(~acc & 0xFFFF);
}

} // namespace toad