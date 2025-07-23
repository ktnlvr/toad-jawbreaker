#pragma once

#include "defs.hpp"
#include <netinet/in.h>

namespace toad {

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

} // namespace toad