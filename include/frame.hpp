#pragma once

#include "defs.hpp"

namespace toad {

struct EthernetFrame {
  u8 dst[6];
  u8 src[6];
  u16 ethertype;

  sz payload_size;
  u8 *payload;
};

} // namespace toad