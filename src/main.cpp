#include "defs.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "arp.hpp"
#include "defs.hpp"
#include "device.hpp"
#include "frame.hpp"

using namespace toad;

int main(void) {
  spdlog::set_level(spdlog::level::trace);

  Device device =
      Device::try_new("toad", "10.12.14.1", "255.255.255.0").value();

  spdlog::debug("(half-life scientist) everything.. seems to be in order");
  spdlog::debug("ARP is expecting {} bytes", ArpIPv4::EXPECTED_LENGTH_BUFFER);

  u8 *buffer = new u8[device.maximum_transmission_unit];
  while (true) {
    ssz n = read(device.fd, buffer, device.maximum_transmission_unit);

    if (n < 0) {
      return errno;
    }

    if (n < 14) {
      spdlog::trace("Received a packet that is too small ({} bytes), skipping",
                    n);
      continue;
    }

    Bytes bytes(buffer, n);
    std::stringstream ss;
    for (int i = 0; i < n; i++)
      ss << std::hex << (int)buffer[i] << ' ';
    spdlog::trace("Bytes: {}", ss.str());

    auto frame = EthernetFrame::from_bytes(bytes).ok();

    spdlog::trace("Processed {} bytes: {}", n, frame);

    if (frame.ethertype == 0x0806) {
      auto arp = ArpIPv4::from_bytes(frame.payload());
      if (arp.is_ok()) {
        auto value = arp.ok();
        spdlog::trace("ARP: {}", value);
      } else {
        spdlog::trace("ARP err: {}", arp.error());
      }
    }
  }

  delete[] buffer;

  return 0;
}