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

  char* buffer = (char*)malloc(0xFFFF);
  while (1) {
    char tapbuffer[4];
    sz nbytes = read(fd, tapbuffer, 4);

    EtherType ethtype = (EtherType)(ntohs(*(uint16_t*)tapbuffer) & 0xFFFF);
    printf("%04hx\n", ethtype);

    EthernetHeader header;

    nbytes = read(fd, buffer, 0xFFFF);

    sz off = 4;
    for (int i = 0; i < 6; i++) header.mac_dst[i] = buffer[off + i];
    off = 10;
    for (int i = 0; i < 6; i++) header.mac_src[i] = buffer[off + i];

    printf("Ethernet Destination: ");
    for (int i = 0; i < 6; i++) printf("%02X ", header.mac_dst[i] & 0xFF);
    printf("\n");

    printf("Ethernet Source: ");
    for (int i = 0; i < 6; i++) printf("%02X ", header.mac_src[i] & 0xFF);
    printf("\n");

    if (nbytes < 0) {
      perror("Reading from interface");
      close(fd);
      exit(1);
    }

    for (int i = off; i < nbytes; i++) printf("%02X ", buffer[i] & 0xFF);
    printf("\n\n");
  }

  return 0;
}
