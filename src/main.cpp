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
#include "icmp.hpp"

using namespace toad;

int main(void) {
  spdlog::set_level(spdlog::level::trace);

  Device device =
      Device::try_new("toad", "10.12.14.1", "255.255.255.0").value();

  spdlog::debug("(half-life scientist) everything.. seems to be in order");

  while (true) {
    auto request = device.read_next_eth();
    spdlog::debug("Received Request: {}", request);

    Bytes bytes(request.payload);
    if (request.ethertype == ETHERTYPE_ARP) {
      auto arp = ArpIPv4<DirectionIn>::try_from_bytes(bytes);

      IPv4 resolved_ip = device.own_ip;
      resolved_ip[3]++;

      if (arp.target_protocol_addr != resolved_ip)
        continue;

      auto arp_response = arp.copy_as_response(device.own_mac);

      spdlog::debug("Arp Request: {}", arp);
      spdlog::debug("Arp Response: {}", arp_response);

      std::vector<u8> arp_payload(
          decltype(arp_response)::EXPECTED_BUFFER_LENGTH);

      bytes = Bytes(arp_payload);
      arp_response.try_to_bytes(bytes);

      MAC fake_mac = device.own_mac;
      fake_mac[0]++;

      auto response = request.clone_as_response(
          ETHERTYPE_ARP, std::move(arp_payload), fake_mac);
      device.write_eth(response);
      spdlog::debug("Sent Request: {}", response);
    } else if (request.ethertype == ETHERTYPE_IPV4) {
      auto ip = Ip<DirectionIn>::try_from_bytes(bytes);
      spdlog::info("{}", ip);
      if (ip.protocol == PROTOCOL_ICMP) {
        Bytes bytes = Bytes(ip.payload);
        auto icmp = Icmp<DirectionOut>::try_from_bytes(bytes);
      }
    }
  }

  return 0;
}