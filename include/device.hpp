#pragma once

#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <fcntl.h>
#include <iostream>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <optional>
#include <sys/ioctl.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include "defs.hpp"
#include "frame.hpp"
#include "mac.hpp"

namespace toad {

struct Device {
  MAC mac;
  int fd;
  sz maximum_transmission_unit;

  u8 *active_eth_packet_data;

  static auto try_new(std::string_view device_name, std::string_view own_ip,
                      std::string_view network_mask) -> std::optional<Device> {
    Device device;

    device.fd = open("/dev/net/tun", O_RDWR);

    ASSERT(device.fd > 0, "File descriptor of /dev/net/tun must be valid");

    ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    std::strncpy(ifr.ifr_name, device_name.data(), IFNAMSIZ);

    if (ioctl(device.fd, TUNSETIFF, (void *)&ifr) < 0) {
      perror("ioctl(TUNSETIFF)");
      close(device.fd);
      return {};
    }

    device.mac = MAC::system_addr();

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
      perror("socket");
      close(device.fd);
      return {};
    }

    {
      ifreq i_addr;
      std::memset(&i_addr, 0, sizeof(i_addr));
      std::strncpy(i_addr.ifr_name, ifr.ifr_name, IFNAMSIZ);
      sockaddr_in *addr = (sockaddr_in *)(&i_addr.ifr_addr);
      addr->sin_family = AF_INET;
      inet_pton(AF_INET, own_ip.data(), &addr->sin_addr);
      if (ioctl(sock_fd, SIOCSIFADDR, &i_addr) < 0) {
        perror("ioctl(SIOCSIFADDR)");
      }

      sockaddr_in *netmask = (sockaddr_in *)(&i_addr.ifr_netmask);
      netmask->sin_family = AF_INET;
      inet_pton(AF_INET, network_mask.data(), &netmask->sin_addr);
      if (ioctl(sock_fd, SIOCSIFNETMASK, &i_addr) < 0) {
        perror("ioctl(SIOCSIFNETMASK)");
      }

      ifreq i_flags;
      std::memset(&i_flags, 0, sizeof(i_flags));
      std::strncpy(i_flags.ifr_name, ifr.ifr_name, IFNAMSIZ);
      if (ioctl(sock_fd, SIOCGIFFLAGS, &i_flags) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
      }

      i_flags.ifr_flags |= (IFF_UP | IFF_RUNNING);
      if (ioctl(sock_fd, SIOCSIFFLAGS, &i_flags) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
      }
    }

    ifreq mtu;
    std::memset(&mtu, 0, sizeof(ifreq));
    std::strncpy(mtu.ifr_name, ifr.ifr_name, IFNAMSIZ);
    if (ioctl(sock_fd, SIOCGIFMTU, &mtu) < 0) {
      perror("ioctl(SIOCGIFMTU)");
      close(sock_fd);
      return {};
    }
    close(sock_fd);

    sz mtu_size = mtu.ifr_mtu;
    device.maximum_transmission_unit = mtu_size;

    spdlog::info("TAP interface {} for device {} at {} with netmask {} created "
                 "with MTU={}",
                 device_name, device.mac, own_ip, network_mask, mtu_size);

    device.active_eth_packet_data = new u8[mtu_size + 14];
    return device;
  }

  auto read_next_eth() -> EthernetFrame<DirectionIn> {
    sz max_packet_size = maximum_transmission_unit + 14;
    ssz n = read(fd, active_eth_packet_data, max_packet_size);

    if (n < 0) {
      std::abort();
    }

    if (n < 14) {
      spdlog::trace("Received a packet that is too small ({} bytes), skipping",
                    n);
      std::abort();
    }

    auto bytes = Bytes(active_eth_packet_data, n);
    return EthernetFrame<DirectionIn>::try_from_bytes(bytes);
  }

  auto write_eth(EthernetFrame<DirectionOut> &frame) {
    std::vector<u8> buffer(frame.min_buffer_size());
    auto bytes = Bytes(buffer);
    frame.try_to_bytes(bytes);
    ssz n = write(fd, buffer.data(), buffer.size());
    spdlog::info("Written {}", n);
  }

  ~Device() {}

protected:
  Device() {}
};

} // namespace toad