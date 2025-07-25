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

using PendingVariant =
    std::variant<PendingRead, PendingReadSome, PendingListen>;

}; // namespace toad