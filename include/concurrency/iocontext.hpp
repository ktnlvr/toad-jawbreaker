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

struct IOContext;

static IOContext *_this_io_context;

auto this_io_context() -> IOContext & { return *_this_io_context; }

struct Socket {
  int _sockfd;
};

enum struct PendingKind { Listen };

struct PendingBase {
  PendingKind kind;

  PendingBase(PendingKind kind) : kind(kind) {}
};

struct PendingListen : PendingBase {
  int sockfd;
  FutureHandle<Socket> handle;

  PendingListen(int sockfd, FutureHandle<Socket> handle)
      : PendingBase(PendingKind::Listen), sockfd(sockfd), handle(handle) {}
};

struct Listener {
  int sockfd;
  u16 port;

  Listener() : sockfd(0), port(0) {}
  Listener(int sockfd, u16 port) : sockfd(sockfd), port(port) {}

  Listener(const Listener &) = delete;
  Listener &operator=(const Listener &) = delete;

  Listener(Listener &&other) {
    sockfd = other.sockfd;
    other.sockfd = 0;
  }

  Listener &operator=(Listener &&other) {
    if (&other == this)
      return *this;
    sockfd = other.sockfd;
    other.sockfd = 0;
    return *this;
  }

  ~Listener() {
    if (sockfd)
      close(sockfd);
  }
};

struct IOContext {
  struct io_uring _ring;

  u32 batch_size, timestamp_ms;

  IOContext(u32 batch_size = 64, u32 timestamp_ms = 5)
      : batch_size(batch_size), timestamp_ms(timestamp_ms) {
    _this_io_context = this;
    if (io_uring_queue_init(64, &_ring, 0) < 0) {
      ASSERT(false, "Failed to init iouring.");
    }
  }

  ~IOContext() { io_uring_queue_exit(&_ring); }

  std::unordered_map<int, PendingBase *> _pending;

  // TODO: error handling
  auto new_listener(u16 port) -> Listener {
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

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

    return Listener(sockfd, port);
  }

  Future<Socket> submit_accept_ipv4(const Listener &listener) {
    auto [future, handle] = Future<Socket>::make_future();

    // OPTIMIZE(Artur): maybe allocate these from some sort of ring/slab
    // allocator or hide the kind in the unused bits of the aligned ptr.
    // A lot of potential here
    PendingBase *pending = new PendingListen(listener.sockfd, handle);

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_accept(sqe, listener.sockfd, NULL, NULL, 0);
    sqe->user_data = (long long)pending;
    // FIXME: check if already enqueued
    _pending[listener.sockfd] = pending;
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

      io_uring_cqe_seen(&_ring, cqe);

      PendingBase *ptr = (PendingBase *)cqe->user_data;
      if (ptr->kind == PendingKind::Listen) {
        auto pending = (PendingListen *)ptr;
        int client_fd = cqe->res;
        spdlog::info("New connection client_fd={}", client_fd);

        auto socket = Socket{._sockfd = client_fd};
        _pending.erase(pending->sockfd);
        pending->handle.set_value(std::move(socket));
      }

      // Re-arm the uring, apparently that's what it's called
      io_uring_submit(&_ring);
    }
  }
};

} // namespace toad