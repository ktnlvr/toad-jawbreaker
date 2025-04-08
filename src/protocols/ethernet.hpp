#pragma once

#include <ostream>

#include "../defs.hpp"
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

void buffer_to_ethernet(byte* buffer, sz length, EthernetFrame* eth_head) {
  memcpy(eth_head->mac_dst, buffer, 6);
  memcpy(eth_head->mac_src, buffer + 6, 6);

  eth_head->eth_type = (EtherType)htons(*(u16*)(buffer + 12));
  eth_head->buffer = buffer + (6 + 6 + 2);

  EXPECT(eth_head->length >= 64,
         "Buffer less than minimum required length (64 bytes)");
  EXPECT(eth_head->length % 4 == 0, "Buffer length is not padded to 4 bytes");

  memcpy(eth_head->crc_checksum, &buffer[length - 4], 4);
}

}  // namespace toad
