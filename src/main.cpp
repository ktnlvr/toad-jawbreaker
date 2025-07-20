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

  MAC fake_mac = device.own_mac;
  fake_mac[0]++;

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

      auto response = request.clone_as_response(
          ETHERTYPE_ARP, std::move(arp_payload), fake_mac);
      device.write_eth(response);
      spdlog::debug("Sent Request: {}", response);
    } else if (request.ethertype == ETHERTYPE_IPV4) {
      auto ip = Ip<DirectionIn>::try_from_bytes(bytes);
      spdlog::info("{} {} {}", ip, ip.header_checksum, ip.calculate_checksum());

      if (ip.protocol == PROTOCOL_ICMP) {
        Bytes bytes = Bytes(ip.payload);
        auto icmp = Icmp<DirectionIn>::try_from_bytes(bytes);
        auto icmp_response = icmp.clone_as_echo_response();

        std::vector<u8> icmp_payload(icmp_response.buffer_size());
        Bytes icmp_payload_bytes = Bytes(icmp_payload);
        icmp_response.try_to_bytes(icmp_payload_bytes);
        spdlog::info("ICMP Payload Size: {}", icmp_payload.size());

        auto ip_response = ip.clone_as_response(std::move(icmp_payload));
        spdlog::info("{}\n{}", icmp_response, ip_response);

        std::vector<u8> ip_payload(ip_response.buffer_size());
        Bytes ip_payload_bytes = Bytes(ip_payload, ip_response.total_length);
        ip_response.try_to_bytes(ip_payload_bytes);

        auto response = request.clone_as_response(
            ETHERTYPE_IPV4, std::move(ip_payload), fake_mac);

        spdlog::info("{}", response);
        device.write_eth(response);
      }
    }
  }

  return 0;
}