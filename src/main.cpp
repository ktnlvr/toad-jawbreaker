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

    MAC dst, src;
    u16 ethertype;

    std::memcpy(dst.data(), buffer, 6);
    std::memcpy(src.data(), buffer + 6, 6);
    ethertype = ntohs(*(u16 *)(buffer + 12));

    sz payload_size = n - 14;

    EthernetFrame frame(dst, src, ethertype, {buffer + 14, payload_size});
    spdlog::trace("Processed {} bytes: {}", n, frame);

    auto arp_res = ArpIPv4::from_buffer({frame.payload, payload_size});

    if (!arp_res.is_ok())
      continue;

    auto arp = arp_res.ok();
    spdlog::trace("{}", arp);
  }

  delete[] buffer;

  return 0;
}