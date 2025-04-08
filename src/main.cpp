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

  byte* buffer = (byte*)malloc(0xFFFF);
  while (1) {
    EthernetFrame ethernet_frame;
    sz nbytes = read(fd, buffer, 0xFFFF);

    // Skip first 4 bytes for the EtherType coming from TUN/TAP
    buffer_to_ethernet(buffer + 4, nbytes - 4, &ethernet_frame);

    printf("MAC DST: ");
    for (int i = 0; i < 6; i++) printf("%02X", ethernet_frame.mac_dst[i]);
    printf("\nMAC SRC: ");
    for (int i = 0; i < 6; i++) printf("%02X", ethernet_frame.mac_src[i]);
    printf("\nDATA:");
    for (int i = 0; i < ethernet_frame.length; i++)
      printf("%02X ", ethernet_frame.buffer[i]);
    printf("\n");

    if (nbytes < 0) {
      perror("Reading from interface");
      close(fd);
      exit(1);
    }
  }

  return 0;
}
