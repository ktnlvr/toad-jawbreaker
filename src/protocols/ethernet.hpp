#pragma once

#include "../defs.hpp"
#include "ethertype.hpp"

namespace toad {

struct EthernetHeader {
  byte mac_dst[6];
  byte mac_src[6];
  EtherType eth_type;
  u32 crc_checksum;
};

}  // namespace toad
