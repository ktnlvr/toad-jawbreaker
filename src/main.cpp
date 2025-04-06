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

int main(void) {
  int fd, err;

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("Opening /dev/net/tun");
    exit(1);
  }

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));

  const char *device_name = "toad";
  strncpy(ifr.ifr_name, device_name, IFNAMSIZ);

  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

  struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
  addr->sin_family = AF_INET;
  if (inet_pton(AF_INET, "10.0.0.1", &addr->sin_addr) <= 0) {
    perror("Invalid IP address");
    return -1;
  }

  if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    exit(1);
  }

  while (1) {
    char buffer[1500];

    unsigned nbytes = read(fd, buffer, sizeof(buffer));
    if (nbytes < 0) {
      perror("Reading from interface");
      close(fd);
      exit(1);
    }

    printf("Received %d bytes\n", nbytes);

    for (int i = 0; i < (nbytes > 16 ? 16 : nbytes); i++)
      printf("%02X ", buffer[i]);
    printf("\n");
  }
}
