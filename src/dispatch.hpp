#pragma once

#include <iostream>
#include <queue>
#include <span>

#include "./protocols/arp.hpp"
#include "./protocols/ethernet.hpp"
#include "defs.hpp"
#include "errc.hpp"

namespace toad {

struct Dispatcher {
  std::queue<std::span<byte>> input_queue, output_queue;

  void enqueue_packet(byte* buffer, sz size) {
    input_queue.push({buffer, size});
  }

  ErrorCode dequeue_packet(byte** out_buffer, sz* size) {
    if (!output_queue.size()) return ErrorCode::NOT_ENOUGH_DATA;
    if (out_buffer == nullptr) return ErrorCode::NULLPTR_OUTPUT;

    auto buf = std::move(output_queue.front());

    *out_buffer = buf.data();
    if (size) *size = buf.size();

    output_queue.pop();

    return ErrorCode::OK;
  }

  ErrorCode process_next() {
    if (!input_queue.size()) return ErrorCode::NOT_ENOUGH_DATA;
    auto errc = ErrorCode::OK;

    auto buf = std::move(input_queue.front());
    input_queue.pop();

    EthernetFrame ethernet_frame;
    buffer_to_ethernet(buf.data(), buf.size(), &ethernet_frame);

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
        goto cleanup;
    };

  cleanup:
    return errc;
  }

  ErrorCode handle_arp(ArpPacket arp) {
    ArpPacket arp_response = arp;
    arp_response.operation = 0x0002;
    memcpy(arp_response.target_mac, arp.sender_mac, 6);
    memcpy(arp_response.target_protocol_addr, arp.sender_protocol_addr, 4);

    // HACK: put the actual MAC here
    memset(arp_response.sender_mac, 0xFF, 6);

    // HACK: put the actual IP here
    byte my_ipv4[4] = {10, 0, 0, 5};
    memcpy(arp_response.sender_protocol_addr, my_ipv4, 6);

    return ErrorCode::OK;
  }
};

}  // namespace toad