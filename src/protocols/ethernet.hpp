#pragma once

#include <ostream>

#include "../defs.hpp"
#include "../errc.hpp"
#include "ethertype.hpp"

namespace toad {

struct EthernetFrame {
  byte mac_dst[6];
  byte mac_src[6];
  EtherType eth_type;
  byte crc_checksum[4];

  byte* buffer;
  u16 length;
};

ErrorCode buffer_to_ethernet(byte* buffer, sz length, EthernetFrame* eth_head) {
  if (length < 6 + 6 + 2 + 4) return ErrorCode::INPUT_BUFFER_TOO_SMALL;

  memcpy(eth_head->mac_dst, buffer, 6);
  memcpy(eth_head->mac_src, buffer + 6, 6);

  eth_head->eth_type = (EtherType)htons(*(u16*)(buffer + 12));
  eth_head->buffer = buffer + (6 + 6 + 2);
  eth_head->length = length - 4 - (6 + 6 + 2);

  memcpy(eth_head->crc_checksum, &buffer[length - 4], 4);

  return ErrorCode::OK;
}

}  // namespace toad
