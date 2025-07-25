#pragma once

#include <array>
#include <cstring>
#include <iostream>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "defs.hpp"

namespace toad {

struct MAC : std::array<u8, 6> {
  using std::array<u8, 6>::array;

  MAC(const std::array<u8, 6> &arr) : std::array<u8, 6>(arr) {}

  static auto system_addr(std::string_view device_name = "eth0") -> MAC {
    MAC ret;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
      return ret;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, device_name.data(), IFNAMSIZ);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
      close(sock);
      return ret;
    }
    close(sock);

    std::memcpy(ret.data(), ifr.ifr_hwaddr.sa_data, 6);
    return ret;
  }

  bool is_default() const {
    bool is_zero = true;
    for (int i = 0; i < 6; i++)
      is_zero &= (*this)[i] == 0;
    return is_zero;
  }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::MAC> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::MAC &f, FormatContext &ctx) const {
    auto out = ctx.out();
    out = format_to(out, "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", f[0],
                    f[1], f[2], f[3], f[4], f[5]);
    return out;
  }
};

} // namespace fmt
