#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "./dispatch.hpp"
#include "./protocols/arp.hpp"
#include "./protocols/ethernet.hpp"
#include "./protocols/ethertype.hpp"

using namespace toad;

int main(void) {
  int fd, err;

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("Opening /dev/net/tun");
    exit(1);
  }

  struct ifreq ifr = {0};
  char device_name[IFNAMSIZ] = "toad";
  strncpy(ifr.ifr_name, device_name, IFNAMSIZ);

  ifr.ifr_flags = IFF_TAP;

  if ((err = ioctl(fd, TUNSETIFF, (void*)&ifr)) < 0) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    exit(1);
  }

  Dispatcher dispatcher;
  NetworkBoundary boundary;

  boundary.init();

  while (1) {
    ErrorCode errc;

    while (!dispatcher.is_queue_empty()) {
      auto packet = dispatcher.dequeue_packet();
      boundary.send(packet);
    }

    byte* buffer = (byte*)malloc(0xFFFF);
    EthernetFrame ethernet_frame;

    const sz nbytes = read(fd, buffer, 0xFFFF);
    if (nbytes < 0) {
      perror("Reading from interface");
      close(fd);
      exit(1);
    }

    dispatcher.enqueue_packet(buffer + 4, nbytes - 4);

    while ((errc = dispatcher.process_next()) != ErrorCode::NOT_ENOUGH_DATA) {
      if (errc != ErrorCode::OK) {
        printf("ERROR WHEN PROCESSING NEXT: %02X\n", errc);
      }
    }
  }

  return 0;
}
