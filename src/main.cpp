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
#include "frame.hpp"

using namespace toad;

int main(void) {
  int fd = open("/dev/net/tun", O_RDWR);

  if (fd < 0) {
    perror("Opening /dev/net/tun");
    return 1;
  }

  ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  std::strncpy(ifr.ifr_name, "toad", IFNAMSIZ);

  if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return 1;
  }

  int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_fd < 0) {
    perror("socket");
    close(fd);
    return 1;
  }

  ifreq mtu;
  std::memset(&mtu, 0, sizeof(ifreq));
  std::strncpy(mtu.ifr_name, ifr.ifr_name, IFNAMSIZ);
  if (ioctl(sock_fd, SIOCGIFMTU, &mtu) < 0) {
    perror("ioctl(SIOCGIFMTU)");
    close(sock_fd);
    return 1;
  }
  size_t mtu_size = mtu.ifr_mtu;
  // 14 from the src, dst and ethertype
  size_t buf_size = mtu_size + 14;

  std::cout << "TAP interface '" << ifr.ifr_name << "' created with MTU "
            << mtu_size << ".\n";

  while (true) {
    u8 *buffer = new u8[mtu_size];
    ssz n = read(fd, buffer, buf_size);
    if (n < 0) {
      return 14;
    }
    if (n < 14) {
      continue;
    }

    EthernetFrame frame;
    std::memcpy(frame.dst, buffer, 6);
    std::memcpy(frame.src, buffer + 6, 6);
    frame.ethertype = ntohs(*(u16 *)(buffer + 12));

    frame.payload_size = n - 14;
    frame.payload = new u8[frame.payload_size];
    std::memcpy(frame.payload, buffer + 14, frame.payload_size);
  }

  close(fd);
  return EXIT_SUCCESS;

  return 0;
}