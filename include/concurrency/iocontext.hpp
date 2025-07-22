#pragma once

#include <liburing.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "future.hpp"

namespace toad {

struct Socket {
  int _sockfd;
};

struct IOContext {
  struct io_uring _ring;

  u32 batch_size, timestamp_ms;

  IOContext(u32 batch_size = 64, u32 timestamp_ms = 5) {
    if (io_uring_queue_init(64, &_ring, 0) < 0) {
      ASSERT(false, "Failed to init iouring.");
    }
  }

  ~IOContext() { io_uring_queue_exit(&_ring); }

  std::unordered_map<int, FutureHandle<Socket>> _listeners;

  Future<Socket> submit_accept_ipv4(u16 port) {
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    auto [future, handle] = Future<Socket>::make_future();

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      ASSERT(false, "Binding the socket failed!");
    }

    // NOTE(Artur): what is the 1?
    if (listen(sockfd, 1) < 0) {
      ASSERT(false, "Listening failed!");
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_accept(sqe, sockfd, NULL, NULL, 0);
    sqe->user_data = sockfd;
    _listeners[sockfd] = handle;
    io_uring_submit(&_ring);
    return future;
  }

  void event_loop() {
    while (true) {
      struct io_uring_cqe *cqe;
      int ret = io_uring_wait_cqe(&_ring, &cqe);
      if (ret < 0) {
        break;
      }

      if (cqe->res < 0) {
        spdlog::error("Error accepting!");
        continue;
      }

      // TODO: check the pending kind

      int client_fd = cqe->res;
      spdlog::info("New connection client_fd={}", client_fd);

      io_uring_cqe_seen(&_ring, cqe);

      struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
      io_uring_prep_accept(sqe, cqe->user_data, NULL, NULL, 0);
      sqe->user_data = cqe->user_data;

      auto socket = Socket{._sockfd = client_fd};
      auto handle = std::move(_listeners[cqe->user_data]);
      handle.set_value(std::move(socket));

      io_uring_submit(&_ring);
    }
  }
};

} // namespace toad