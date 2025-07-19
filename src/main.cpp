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

#include "arp.hpp"
#include "defs.hpp"
#include "device.hpp"
#include "frame.hpp"

using namespace toad;

int main(void) {
  spdlog::set_level(spdlog::level::trace);

  Device device =
      Device::try_new("toad", "10.12.14.1", "255.255.255.0").value();

  spdlog::debug("(half-life scientist) everything.. seems to be in order");

  while (true) {
    auto result = device.read_next_eth();
  }

  return 0;
}