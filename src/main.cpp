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

#include "concurrency/channel.hpp"
#include "concurrency/executor.hpp"
#include "concurrency/future.hpp"
#include "concurrency/iocontext.hpp"
#include "concurrency/task.hpp"

using namespace toad;

Task<void> client_reader(const Socket &&sock) {
  IOContext &io = this_io_context();
  auto rx = io.submit_read(sock);

  std::optional<u8> data;
  do {
    data = co_await rx.recv();
    if (data.has_value()) {
      spdlog::info("Received byte: 0x{:02X}", data.value());
      io.submit_write_some(sock, std::span(&data.value(), 1));
    } else {
      spdlog::info("Terminating connection...");
    }
  } while (data.has_value());
}

Task<void> client_acceptor(const Listener &listener) {
  IOContext &io = this_io_context();

  while (true) {
    spdlog::info("Waiting for a socket...");
    auto client = co_await io.submit_accept_ipv4(listener);
    spdlog::info("Omg! Got client, fd = {}", client._sockfd);

    spawn(client_reader(std::move(client)));
  }
}

int main(void) {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%L] [thread %t] %v");

  IOContext io_ctx;
  Executor executor;

  // Create a listener on port 8080
  auto listener = io_ctx.new_listener(8080);
  spdlog::info("Server listening on port 8080...");

  // Spawn the client acceptor task
  spawn(client_acceptor(listener));

  // Run the event loop
  io_ctx.event_loop();

  return 0;
}