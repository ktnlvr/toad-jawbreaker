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
#include "nic/arp.hpp"
#include "nic/device.hpp"
#include "nic/ethernet.hpp"
#include "nic/icmp.hpp"

#include "concurrency.hpp"

#include "socks5/server.hpp"

using namespace toad;

int main(void) {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%L] [thread %t] %v");

  IOContext io_ctx;
  Executor executor;

  socks5::Socks5Server server;
  spawn(std::move(server).serve_socks5());

  // Run the event loop
  io_ctx.event_loop();

  return 0;
}