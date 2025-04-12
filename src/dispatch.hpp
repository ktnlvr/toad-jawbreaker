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
    if ((errc = buffer_to_ethernet(buf.data(), buf.size(), &ethernet_frame)) !=
        ErrorCode::OK)
      return errc;

    switch (ethernet_frame.eth_type) {
      case EtherType::ARP: {
        ArpPacket arp;
        errc =
            buffer_to_arp(ethernet_frame.buffer, ethernet_frame.length, &arp);
        if (errc != ErrorCode::OK) goto cleanup;
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
    memcpy(arp_response.sender_protocol_addr, my_ipv4, 6);

    int sockfd;
    struct sockaddr_ll dest_addr = {0};
    byte frame[ETH_HLEN + 28];

    if ((sockfd = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL)) < 0) {
      return ErrorCode::SOCKET_INIT_FAILED;
    }

    struct ifreq if_idx = {0};
    strncpy(if_idx.ifr_name, "toad", IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
      close(sockfd);
      return ErrorCode::SOCKET_SETUP_FAILED;
    }

    struct ethhdr* eth = (struct ethhdr*)frame;
    memcpy(eth->h_dest, arp_response.target_mac, ETH_ALEN);
    memcpy(eth->h_source, arp_response.sender_mac, ETH_ALEN);
    eth->h_proto = htons(ETH_P_ARP);

    arp_response.write_to_buffer(frame + ETH_HLEN, 28);

    dest_addr.sll_family = AF_PACKET;
    dest_addr.sll_ifindex = if_idx.ifr_ifindex;
    dest_addr.sll_halen = ETH_ALEN;
    memcpy(dest_addr.sll_addr, eth->h_dest, ETH_ALEN);

    sz nbytes = sendto(sockfd, frame, sizeof(frame), 0, (sockaddr*)&dest_addr,
                       sizeof(dest_addr));

    if (nbytes < 0) return ErrorCode::SOCKET_WRITE_FAILED;

    close(sockfd);

    return ErrorCode::OK;
  }
};

}  // namespace toad