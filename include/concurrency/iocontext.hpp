#pragma once

#include <liburing.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../net/listener.hpp"
#include "../net/socket.hpp"

#include "channel.hpp"
#include "future.hpp"

namespace toad {

struct IOContext;

static IOContext *_this_io_context;

auto this_io_context() -> IOContext & { return *_this_io_context; }

enum struct PendingKind { Listen, Receive, Read };

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

struct PendingReceive : PendingBase {
  int sockfd;
  FutureHandle<std::optional<Buffer>> handle;
  Buffer buffer;

  PendingReceive(int sockfd, Buffer buffer,
                 FutureHandle<std::optional<Buffer>> handle)
      : PendingBase(PendingKind::Receive), sockfd(sockfd), buffer(buffer),
        handle(handle) {}
};

struct PendingRead : PendingBase {
  int sockfd;
  TX<u8> tx;
  std::array<u8, 4096> buffer;
  bool active;

  PendingRead(int sockfd, TX<u8> tx)
      : PendingBase(PendingKind::Read), sockfd(sockfd), tx(std::move(tx)),
        active(true) {}
};

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

  std::unordered_map<int, PendingListen *> _pending_listeners;
  std::unordered_map<int, PendingReceive *> _pending_recv;
  std::unordered_map<int, PendingRead *> _pending_read;

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
    auto pending = new PendingListen(listener.sockfd, handle);

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_accept(sqe, listener.sockfd, NULL, NULL, 0);
    sqe->user_data = (long long)pending;
    // FIXME: check if already enqueued
    _pending_listeners[listener.sockfd] = pending;

    return std::move(future);
  }

  /// @returns Buffer of size max_size or smaller when data is received.
  /// std::nullopt when the connection is shut down
  Future<std::optional<Buffer>> submit_read_some(const Socket &socket,
                                                 sz max_size) {
    Buffer buffer(max_size);
    auto [future, handle] = Future<std::optional<Buffer>>::make_future();
    auto pending = new PendingReceive(socket._sockfd, buffer, handle);

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_read(sqe, socket._sockfd, buffer.data(), max_size, 0);
    sqe->user_data = (long long)pending;
    _pending_recv[socket._sockfd] = pending;

    return std::move(future);
  }

  /// @returns RX<u8> channel for continuous reading from socket
  RX<u8> submit_read(const Socket &socket) {
    auto [tx, rx] = channel<u8>(8192);
    auto pending = new PendingRead(socket._sockfd, std::move(tx));

    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_read(sqe, socket._sockfd, pending->buffer.data(),
                       pending->buffer.size(), 0);
    sqe->user_data = (long long)pending;
    _pending_read[socket._sockfd] = pending;

    return std::move(rx);
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

        cqes[seen++] = cqe;
      }

      seen = io_uring_peek_batch_cqe(&_ring, cqes.data(), batch_size);

      for (int i = 0; i < seen; i++) {
        struct io_uring_cqe *cqe = cqes[i];

        io_uring_cqe_seen(&_ring, cqe);
        if (cqe->res < 0) {
          spdlog::error("uring op failed: {}", errno);
          continue;
        }

        PendingBase *ptr = (PendingBase *)cqe->user_data;
        if (ptr->kind == PendingKind::Listen) {
          auto pending = (PendingListen *)ptr;
          int client_fd = cqe->res;
          spdlog::info("New connection client_fd={}", client_fd);

          auto socket = Socket(client_fd);
          _pending_listeners.erase(pending->sockfd);
          pending->handle.set_value(std::move(socket));
        } else if (ptr->kind == PendingKind::Receive) {
          auto pending = (PendingReceive *)ptr;
          int client_fd = pending->sockfd;

          // TODO: check if the socket closd
          int read = cqe->res;
          _pending_recv.erase(pending->sockfd);
          spdlog::info("Read {} bytes from client_fd={}", read, client_fd);

          std::optional<Buffer> value =
              read == 0 ? std::nullopt
                        : std::optional(std::move(pending->buffer.slice(read)));
          pending->handle.set_value(std::move(value));
        } else if (ptr->kind == PendingKind::Read) {
          auto pending = (PendingRead *)ptr;
          int client_fd = pending->sockfd;
          int read = cqe->res;

          if (read > 0 && pending->active) {
            for (int i = 0; i < read; i++) {
              if (!pending->tx.send(std::move(pending->buffer[i]))) {
                pending->active = false;
                break;
              }
            }

            if (pending->active) {
              struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
              io_uring_prep_read(sqe, client_fd, pending->buffer.data(),
                                 pending->buffer.size(), 0);
              sqe->user_data = (long long)pending;
            } else {
              _pending_read.erase(client_fd);
              delete pending;
            }
          } else {
            _pending_read.erase(client_fd);
            delete pending;
          }
        }
      }
    }
  }

  ~IOContext() { io_uring_queue_exit(&_ring); }
};

} // namespace toad