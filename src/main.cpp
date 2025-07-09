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

#include "defs.hpp"
#include "device.hpp"
#include "frame.hpp"

using namespace toad;

int main(void) {
  spdlog::set_level(spdlog::level::trace);

  Device device =
      Device::try_new("toad", "10.12.14.1", "255.255.255.0").value();

  spdlog::debug("(half-life scientist) everything.. seems to be in order");

  u8 *buffer = new u8[device.maximum_transmission_unit];
  while (true) {
    ssz n = read(device.fd, buffer, device.maximum_transmission_unit);

    if (n < 0) {
      std::cout << errno << std::endl;
      return 14;
    }
    if (n < 14) {
      spdlog::trace("Received a packet that is too small ({} bytes), skipping",
                    n);
      continue;
    }

    EthernetFrame frame;
    std::memcpy(frame.dst, buffer, 6);
    std::memcpy(frame.src, buffer + 6, 6);
    frame.ethertype = ntohs(*(u16 *)(buffer + 12));

    frame.payload_size = n - 14;
    frame.payload = new u8[frame.payload_size];
    std::memcpy(frame.payload, buffer + 14, frame.payload_size);

    spdlog::trace("Processed {} bytes: {}", n, frame);
  }

  return EXIT_SUCCESS;

  return 0;
}