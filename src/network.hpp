#pragma once

#include <vector>

#include "./protocols/ethertype.hpp"
#include "defs.hpp"

namespace toad {

struct BoundaryPacket {
  BoundaryPacket(byte mac_dst[6], byte mac_src[6], EtherType protocol,
                 std::vector<byte> buffer)
      : protocol(protocol), buffer(buffer) {
    memcpy(mac_destination, mac_dst, 6);
    memcpy(mac_source, mac_src, 6);
  }

  byte mac_destination[6];
  byte mac_source[6];

  EtherType protocol;
  std::vector<byte> buffer;

  std::pair<byte*, sz> data() { return {buffer.data(), buffer.size()}; }
};

// Immutable context used for Network interactions
struct NetworkContext {
  byte my_mac[6] = {};
};

struct NetworkBoundary {
  static constexpr sz active_buffer_size = 0xFFFF;

  int _sockfd = 0;
  int _interface_index = 0;

  byte* _active_buffer = nullptr;

  NetworkBoundary() {
    _active_buffer = (byte*)malloc(active_buffer_size);
    memset(_active_buffer, 0xCC, active_buffer_size);
  }

  NetworkBoundary(NetworkBoundary&&) = delete;
  NetworkBoundary(const NetworkBoundary&) = delete;

  ~NetworkBoundary() {
    if (_sockfd) close(_sockfd);
    if (_active_buffer) free(_active_buffer);
  }

  // HACK: output parameter, replace with a proper result in the future
  ErrorCode init(NetworkContext* out) {
    if ((_sockfd = socket(AF_PACKET, SOCK_RAW, ETH_P_ALL)) < 0) {
      return ErrorCode::SOCKET_INIT_FAILED;
    }

    struct ifreq interface_request = {0};
    strncpy(interface_request.ifr_name, "toad", IFNAMSIZ - 1);
    if (ioctl(_sockfd, SIOCGIFINDEX, &interface_request) < 0) {
      close(_sockfd);
      return ErrorCode::SOCKET_SETUP_FAILED;
    }

    _interface_index = interface_request.ifr_ifindex;
    if (ioctl(_sockfd, SIOCGIFHWADDR, &interface_request) < 0) {
      close(_sockfd);
      return ErrorCode::SOCKET_SETUP_FAILED;
    }

    memcpy(out->my_mac, interface_request.ifr_hwaddr.sa_data, 6);

    return ErrorCode::OK;
  }

  ErrorCode send(BoundaryPacket packet) {
    if (_sockfd == 0) return ErrorCode::NON_RAII_OBJECT_UNINITIALIZED;
    // TODO: check if the data fits in the active buffer

    struct ethhdr* eth = (struct ethhdr*)_active_buffer;
    byte* payload_buffer = _active_buffer + ETH_HLEN;

    memcpy(eth->h_dest, packet.mac_destination, 6);
    memcpy(eth->h_source, packet.mac_source, 6);
    eth->h_proto = htons((u16)packet.protocol);

    auto [data, size] = packet.data();
    memcpy(payload_buffer, data, size);

    struct sockaddr_ll dest_addr = {0};
    dest_addr.sll_family = AF_PACKET;
    dest_addr.sll_ifindex = _interface_index;
    dest_addr.sll_halen = ETH_ALEN;
    dest_addr.sll_protocol = eth->h_proto;
    memcpy(dest_addr.sll_addr, eth->h_dest, ETH_ALEN);

    auto buffer_size = size + ETH_HLEN;

    sz nbytes = sendto(_sockfd, _active_buffer, buffer_size, 0,
                       (sockaddr*)&dest_addr, sizeof(dest_addr));

    if (nbytes < 0) return ErrorCode::SOCKET_WRITE_FAILED;

    reset_active_buffer();

    return ErrorCode::OK;
  }

  void reset_active_buffer() {
    memset(this->_active_buffer, 0xCC, active_buffer_size);
  }
};

}  // namespace toad