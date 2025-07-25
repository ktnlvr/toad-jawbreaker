#pragma once

#include <liburing.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_set>

#include "../net/listener.hpp"
#include "../net/socket.hpp"

#include "channel.hpp"
#include "future.hpp"
#include "pending.hpp"

namespace toad {

struct IOContext;

static IOContext *_this_io_context;

auto this_io_context() -> IOContext & { return *_this_io_context; }

struct IOContext {
  struct io_uring _ring;

  u32 batch_size, timeout_ms;

  IOContext(u32 batch_size = 64, u32 timeout_ms = 5)
      : batch_size(batch_size), timeout_ms(timeout_ms) {
    _this_io_context = this;
    if (io_uring_queue_init(batch_size, &_ring, 0) < 0) {
      ASSERT(false, "Failed to init iouring.");
    }
  }

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
    PendingVariant *pending =
        new PendingVariant(PendingListen(listener.sockfd, handle));

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_accept(sqe, listener.sockfd, NULL, NULL, 0);
    sqe->user_data = (long long)pending;

    return std::move(future);
  }

  /// @returns Buffer of size max_size or smaller when data is received.
  /// std::nullopt when the connection is shut down
  Future<std::optional<Buffer>> submit_read_some(const Socket &socket,
                                                 sz max_size) {
    Buffer buffer(max_size);
    auto [future, handle] = Future<std::optional<Buffer>>::make_future();
    PendingVariant *pending =
        new PendingVariant(PendingReadSome(socket._sockfd, buffer, handle));

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_read(sqe, socket._sockfd, buffer.data(), max_size, 0);
    sqe->user_data = (long long)pending;

    return std::move(future);
  }

  /// @returns RX<u8> channel for continuous reading from socket
  RX<u8> submit_read(const Socket &socket) {
    auto [tx, rx] = channel<u8>(8192);

    PendingVariant *pending =
        new PendingVariant(PendingRead(socket._sockfd, std::move(tx)));

    // SAFETY(Artur): we just stored it, so we can be 100% sure its that type
    PendingRead &read = std::get<PendingRead>(*pending);

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_read(sqe, socket._sockfd, read.buffer.data(),
                       read.buffer.size(), 0);
    sqe->user_data = (long long)pending;

    return std::move(rx);
  }

  template <sz spansize>
  void submit_write_some(const Socket &socket, std::span<u8, spansize> buffer) {

    sz size = buffer.size();
    u8 *buf = new u8[size];
    memcpy(buf, buffer.data(), size);

    PendingVariant *pending =
        new PendingVariant(PendingWriteSome(socket._sockfd, buf));

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_write(sqe, socket._sockfd, buf, size, 0);
    sqe->user_data = (long long)pending;
  }

  bool _handle_pending(struct io_uring_cqe *cqe, PendingRead &read) {
    int client_fd = read.sockfd;
    int bytes_read = cqe->res;

    if (bytes_read > 0 && read.active) {
      for (int i = 0; i < bytes_read; i++) {
        if (!read.tx.send(std::move(read.buffer[i]))) {
          read.active = false;
          break;
        }
      }

      if (read.active) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
        io_uring_prep_read(sqe, client_fd, read.buffer.data(),
                           read.buffer.size(), 0);
        sqe->user_data = cqe->user_data;
        return true;
      }
    }

    return false;
  }

  bool _handle_pending(struct io_uring_cqe *cqe, PendingReadSome &read_some) {
    int client_fd = read_some.sockfd;

    // TODO: check if the socket closd
    int read = cqe->res;
    spdlog::info("Read {} bytes from client_fd={}", read, client_fd);

    std::optional<Buffer> value =
        read == 0 ? std::nullopt
                  : std::optional(std::move(read_some.buffer.slice(read)));
    read_some.handle.set_value(std::move(value));
    return false;
  }

  bool _handle_pending(struct io_uring_cqe *cqe, PendingListen &listen) {
    int client_fd = cqe->res;
    spdlog::info("New connection client_fd={}", client_fd);

    auto socket = Socket(client_fd);
    listen.handle.set_value(std::move(socket));
    return false;
  }

  bool _handle_pending(struct io_uring_cqe *cqe, PendingWriteSome &write_some) {
    if (cqe->res < 0)
      spdlog::error("Write broke, code={}", errno);

    return false;
  }

  void event_loop() {
    struct __kernel_timespec ts;
    ts.tv_sec = timeout_ms / 1000;
    ts.tv_nsec = (timeout_ms % 1000) * 1000000;

    std::vector<struct io_uring_cqe *> cqes(batch_size);

    // TODO: graceful shutdown
    while (true) {
      io_uring_submit(&_ring);

      int seen = 0;

      {
        struct io_uring_cqe *cqe;
        // Wait for timeout or just one cqe
        int ret = io_uring_wait_cqe_timeout(&_ring, &cqe, &ts);
        if (ret < 0) {
          if (ret == -ETIME) {
            // no events arrived, just loop again
            continue;
          } else {
            // fatal error
            spdlog::error("io_uring_wait_cqe_timeout failed: {}", errno);
            break;
          }
        }
      }

      seen = io_uring_peek_batch_cqe(&_ring, cqes.data(), batch_size);

      for (int i = 0; i < seen; i++) {
        struct io_uring_cqe *cqe = cqes[i];

        io_uring_cqe_seen(&_ring, cqe);
        if (cqe->res < 0) {
          spdlog::error("uring op failed: {}", errno);
          continue;
        }

        auto pending = (PendingVariant *)cqe->user_data;
        bool keep_alive = std::visit(
            [&](auto &pending) { return _handle_pending(cqe, pending); },
            *pending);

        if (!keep_alive) {
          delete pending;
        }
      }
    }
  }

  ~IOContext() { io_uring_queue_exit(&_ring); }
};

} // namespace toad