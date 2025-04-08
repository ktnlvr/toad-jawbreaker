#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../defs.hpp"
#include "../errc.hpp"

namespace toad {

struct ArpPacket {
  u16 hardware_type;
  u16 protocol_type;

  // Variable in the protocol but expected to be
  // constants here
  byte mac_addr_length;       // 6
  byte protocol_addr_length;  // gonna be either IPv6 or IPv4, so 4 or 6

  u16 operation;
  byte sender_mac[6];

  // HACK: the size might change depending on IPv4 and IPv6. Requires further
  // investigation
  byte sender_protocol_addr[4];

  byte target_mac[6];
  byte target_protocol_addr[4];
};

// Expects a payload buffer of the ethernet frame
ErrorCode buffer_to_arp(byte* buffer, sz length, ArpPacket* arp_packet) {
  // Smallest possible is an Ethernet + IPv4 header, it's 28 bytes
  if (length < 28) return ErrorCode::INPUT_BUFFER_TOO_SMALL;

  arp_packet->hardware_type = htons(*buffer);
  arp_packet->protocol_type = htons(*(buffer + 2));

  arp_packet->mac_addr_length = *(buffer + 4);
  arp_packet->protocol_addr_length = *(buffer + 5);

  arp_packet->operation = htons(*(buffer + 6));

  memcpy(arp_packet->sender_mac, buffer + 8, 6);
  memcpy(arp_packet->sender_protocol_addr, buffer + 14, 4);
  memcpy(arp_packet->target_mac, buffer + 18, 6);
  memcpy(arp_packet->target_protocol_addr, buffer + 24, 4);

  return ErrorCode::OK;
}

}  // namespace toad
