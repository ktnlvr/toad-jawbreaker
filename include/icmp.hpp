#pragma once

#include "bytes.hpp"
#include "typestate.hpp"

#include <span>

namespace toad {

template <TypestateDirection direction> struct Icmp {
  u8 type;
  u8 code;
  u8 checksum;
  u16 identifier;
  u16 sequence_number;
  u8 *payload;
  std::span<u8> payload;
};

} // namespace toad