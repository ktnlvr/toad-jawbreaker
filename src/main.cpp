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
#include "nic/arp.hpp"
#include "nic/device.hpp"
#include "nic/ethernet.hpp"
#include "nic/icmp.hpp"

using namespace toad;

int main(void) {
  spdlog::set_level(spdlog::level::trace);

  Device device =
      Device::try_new("toad", "10.12.14.1", "255.255.255.0").value();

  MAC fake_mac = device.own_mac;
  fake_mac[0]++;

  IPv4 resolve_ip = device.own_ip;
  resolve_ip[3]++;

  spdlog::debug("(half-life scientist) everything.. seems to be in order");

  while (true) {
    auto request = device.read_next_eth();
    spdlog::debug("Received Request: {}", request);

    ByteIStream ethernet_payload_istream = request.payload;
    if (request.ethertype == ETHERTYPE_ARP) {
      auto arp =
          ArpIPv4<DirectionIn>::try_from_stream(ethernet_payload_istream);

      spdlog::info("{}", arp);

      if (arp.target_protocol_addr != resolve_ip)
        continue;

      auto arp_response = arp.copy_as_response(device.own_mac);

      spdlog::debug("Arp Request: {}", arp);
      spdlog::debug("Arp Response: {}", arp_response);

      sz arp_payload_size = decltype(arp_response)::EXPECTED_BUFFER_LENGTH;
      auto response =
          request.clone_as_response(ETHERTYPE_ARP, arp_payload_size, fake_mac);

      ByteOStream ethernet_response_ostream = response.payload;
      arp_response.try_to_stream(ethernet_response_ostream);

      spdlog::info("{}", response);

      device.write_eth(response);
      spdlog::debug("Sent Request: {}", response);
    } else if (request.ethertype == ETHERTYPE_IPV4) {
      auto ip = Ip<DirectionIn>::try_from_stream(ethernet_payload_istream);
      spdlog::info("{} {} {}", ip, ip.header_checksum, ip.calculate_checksum());

      if (ip.protocol == PROTOCOL_ICMP) {
        auto ip_payload_istream = ByteIStream(ip.payload);
        auto icmp = Icmp<DirectionIn>::try_from_stream(ip_payload_istream);
        auto icmp_response = icmp.clone_as_echo_response();

        auto icmp_response_size = icmp_response.buffer_size();
        auto ip_payload_size = IP_HEADER_SIZE + icmp_response_size;

        spdlog::info("Payload size! {}", ip_payload_size);

        Buffer icmp_raw_buffer(icmp_response.buffer_size());
        ByteOStream icmp_ostream = ByteOStream(icmp_raw_buffer);
        icmp_response.try_to_stream(icmp_ostream);

        std::vector<u8> ip_payload_raw(icmp_raw_buffer.size);
        ByteIStream(icmp_raw_buffer).read_vector(&ip_payload_raw);

        auto ip_response = ip.clone_as_response(std::move(ip_payload_raw));
        spdlog::info("{}", ip_response.payload);

        auto eth_response = request.clone_as_response(
            ETHERTYPE_IPV4, ip_payload_size, fake_mac);
        ByteOStream eth_response_payload_ostream = eth_response.payload;
        ip_response.try_to_stream(eth_response_payload_ostream);
        spdlog::info("{}", eth_response);
        device.write_eth(eth_response);
      }
    }
  }

  return 0;
}