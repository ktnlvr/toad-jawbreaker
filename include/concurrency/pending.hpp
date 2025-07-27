#pragma once

#include <variant>

#include "../bytes/buffer.hpp"
#include "../net/socket.hpp"
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
  FutureHandle<std::optional<Socket>> handle;

  PendingConnect(int sockfd, struct sockaddr_in addr,
                 FutureHandle<std::optional<Socket>> handle)
      : sockfd(sockfd), addr(addr), handle(handle) {}
};

struct PendingReadSomeVec {
  int sockfd;
  FutureHandle<sz> handle;
  std::vector<u8> &vec;
  sz initial_size;

  PendingReadSomeVec(int sockfd, std::vector<u8> &vec, sz initial_size,
                     FutureHandle<sz> handle)
      : sockfd(sockfd), vec(vec), handle(handle) {}
};

struct PendingReadSome {
  int sockfd;
  FutureHandle<std::optional<Buffer>> handle;
  Buffer buffer;

  PendingReadSome(int sockfd, Buffer buffer,
                  FutureHandle<std::optional<Buffer>> handle)
      : sockfd(sockfd), buffer(buffer), handle(handle) {}
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

using PendingVariant =
    std::variant<PendingReadSome, PendingListen, PendingConnect,
                 PendingWriteSome, PendingReadSomeVec>;

}; // namespace toad