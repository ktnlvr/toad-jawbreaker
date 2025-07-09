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
  Device device = Device::try_new("10.12.14.1", "255.255.255.0").value();

  while (true) {
    u8 *buffer = new u8[device.maximum_transmission_unit];
    ssz n = read(device.fd, buffer, device.maximum_transmission_unit);

    if (n < 0) {
      std::cout << errno << std::endl;
      return 14;
    }
    if (n < 14) {
      continue;
    }

    EthernetFrame frame;
    std::memcpy(frame.dst, buffer, 6);
    std::memcpy(frame.src, buffer + 6, 6);
    frame.ethertype = ntohs(*(u16 *)(buffer + 12));

    frame.payload_size = n - 14;
    frame.payload = new u8[frame.payload_size];
    std::memcpy(frame.payload, buffer + 14, frame.payload_size);
  }

  return EXIT_SUCCESS;

  return 0;
}