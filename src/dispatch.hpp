#pragma once

#include <linux/if_arp.h>
#include <net/ethernet.h>
#include <sys/socket.h>

#include <iostream>
#include <queue>
#include <span>

#include "./protocols/arp.hpp"
#include "./protocols/ethernet.hpp"
#include "defs.hpp"
#include "errc.hpp"
#include "network.hpp"

namespace toad {

struct Dispatcher {
  std::queue<std::vector<byte>> input_queue;
  std::queue<BoundaryPacket> output_queue;

  void enqueue_packet(byte* buffer, sz size) {
    std::vector<byte> data(size);
    memcpy(data.data(), buffer, size);
    input_queue.push(data);
  }

  auto is_queue_empty() -> bool { return !output_queue.size(); }

  BoundaryPacket dequeue_packet() {
    auto packet = std::move(output_queue.front());
    output_queue.pop();
    return packet;
  }

  ErrorCode process_next() {
    if (!input_queue.size()) return ErrorCode::NOT_ENOUGH_DATA;
    auto errc = ErrorCode::OK;

    auto buf = std::move(input_queue.front());
    input_queue.pop();

    EthernetFrame ethernet_frame;
    if ((errc = buffer_to_ethernet(buf.data(), buf.size(), &ethernet_frame)) !=
        ErrorCode::OK)
      return errc;

    switch (ethernet_frame.eth_type) {
      case EtherType::ARP: {
        ArpPacket arp;
        errc =
            buffer_to_arp(ethernet_frame.buffer, ethernet_frame.length, &arp);
        handle_arp(arp);

        break;
      }
      default:
        // WARN: memory leak
        errc = ErrorCode::CODE_PATH_NOT_HANDLED;
    };

    return errc;
  }

  ErrorCode handle_arp(ArpPacket arp_request) {
    if (arp_request.operation == 2) return ErrorCode::OK;

    ArpPacket arp_response = arp_request;
    arp_response.operation = 0x0002;
    memcpy(arp_response.target_mac, arp_request.sender_mac, 6);
    memcpy(arp_response.target_protocol_addr, arp_request.sender_protocol_addr,
           4);

    // HACK: put the actual MAC here
    memset(arp_response.sender_mac, 0xFF, 6);

    // HACK: put the actual IP here
    byte my_ipv4[4] = {10, 0, 0, 1};
    memcpy(arp_response.sender_protocol_addr, my_ipv4, 4);

    std::vector<byte> arp_data(28);
    arp_response.write_to_buffer(arp_data.data(), 28);

    auto packet =
        BoundaryPacket(arp_response.target_mac, arp_response.sender_mac,
                       EtherType::ARP, arp_data);

    output_queue.push(packet);

    return ErrorCode::OK;
  }
};

}  // namespace toad