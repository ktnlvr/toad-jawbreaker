#pragma once

#include <variant>

#include "../bytes/buffer.hpp"
#include "../net/socket.hpp"
#include "channel.hpp"
#include "future.hpp"

namespace toad {

struct PendingListen {
  int sockfd;
  FutureHandle<Socket> handle;

  PendingListen(int sockfd, FutureHandle<Socket> handle)
      : sockfd(sockfd), handle(handle) {}
};

struct PendingConnect {
  int sockfd;
  struct sockaddr_in addr;
  FutureHandle<Socket> handle;

  PendingConnect(int sockfd, struct sockaddr_in addr,
                 FutureHandle<Socket> handle)
      : sockfd(sockfd), addr(addr), handle(handle) {}
};

struct PendingReadSome {
  int sockfd;
  FutureHandle<std::optional<Buffer>> handle;
  Buffer buffer;

  PendingReadSome(int sockfd, Buffer buffer,
                  FutureHandle<std::optional<Buffer>> handle)
      : sockfd(sockfd), buffer(buffer), handle(handle) {}
};

struct PendingRead {
  int sockfd;
  TX<u8> tx;
  std::array<u8, 4096> buffer;
  bool active;

  PendingRead(int sockfd, TX<u8> tx)
      : sockfd(sockfd), tx(std::move(tx)), active(true) {}
};

struct PendingWriteSome {
  int sockfd;
  u8 *buffer;

  PendingWriteSome(int sockfd, u8 *buffer) : sockfd(sockfd), buffer(buffer) {}

  PendingWriteSome(const PendingWriteSome &pw) = delete;
  PendingWriteSome &operator=(const PendingWriteSome &pw) = delete;

  PendingWriteSome(PendingWriteSome &&pw) : sockfd(pw.sockfd) {
    this->buffer = pw.buffer;
    pw.buffer = nullptr;
  }

  PendingWriteSome &operator=(PendingWriteSome &&pw) {
    if (this == &pw)
      return *this;

    this->sockfd = pw.sockfd;
    this->buffer = pw.buffer;
    pw.buffer = nullptr;

    return *this;
  }

  ~PendingWriteSome() {
    if (buffer)
      delete[] buffer;
  }
};

using PendingVariant = std::variant<PendingRead, PendingReadSome, PendingListen,
                                    PendingConnect, PendingWriteSome>;

}; // namespace toad