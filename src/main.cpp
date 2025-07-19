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

  while (true) {
    auto request = device.read_next_eth();
    spdlog::debug("Received Request: {}", request);

    if (request.ethertype == ETHERTYPE_ARP) {
      Bytes bytes(request.payload);
      auto arp = ArpIPv4::try_from_bytes(bytes);
      if (arp.target_protocol_addr != IPv4({10, 12, 14, 99}))
        continue;

      auto arp_response = arp.clone_as_response(device.mac);

      spdlog::debug("Arp Request: {}", arp);
      spdlog::debug("Arp Response: {}", arp_response);

      std::vector<u8> arp_payload(ArpIPv4::EXPECTED_BUFFER_LENGTH);

      bytes = Bytes(arp_payload);
      arp_response.try_to_bytes(bytes);

      MAC fake_mac = device.mac;
      fake_mac[0]++;

      auto response = request.clone_as_response(
          ETHERTYPE_ARP, std::move(arp_payload), fake_mac);
      device.write_eth(response);
      spdlog::debug("Sent Request: {}", response);
    }
  }

  return 0;
}