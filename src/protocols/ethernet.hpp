#pragma once

#include <ostream>

#include "../defs.hpp"
#include "../errc.hpp"
#include "ethertype.hpp"

namespace toad {

struct EthernetFrame {
  static constexpr sz STATIC_DATA_SIZE = 14;

  byte mac_dst[6];
  byte mac_src[6];
  EtherType eth_type;

  byte* buffer;
  u16 length;
};

ErrorCode buffer_to_ethernet(byte* buffer, sz length,
                             EthernetFrame* ethernet_frame) {
  if (length < 6 + 6 + 2 + 4) return ErrorCode::BUFFER_TOO_SMALL;

  memcpy(ethernet_frame->mac_dst, buffer, 6);
  memcpy(ethernet_frame->mac_src, buffer + 6, 6);

  ethernet_frame->eth_type = (EtherType)ntohs(*(u16*)(buffer + 6 + 6));
  ethernet_frame->buffer = buffer + (6 + 6 + 2);
  ethernet_frame->length = length - (6 + 6 + 2);

  return ErrorCode::OK;
}

}  // namespace toad
