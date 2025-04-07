#pragma once

#include "../defs.hpp"

namespace toad {

enum struct EtherType : u16 {
  IPv6 = 0x0800,
  ARP = 0x0806,
};

sz buf_size_from_ethertype(EtherType et) {
  switch (et) {
    case EtherType::IPv6:
      return 0xFFFF;
    case EtherType::ARP:
      return 0xFFFF;
    default:
      return 0;
  }
}

}  // namespace toad
