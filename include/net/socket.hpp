#pragma once

namespace toad {

struct Socket {
  int _sockfd = -1;

  Socket(int sockfd) : _sockfd(sockfd) {}

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  Socket(Socket &&other) {
    spdlog::debug("AAAAAAA");
    if (this == &other)
      return;
    if (_sockfd != -1)
      close(_sockfd);
    _sockfd = other._sockfd;
    other._sockfd = -1;
  }

  Socket &operator=(Socket &&other) {
    spdlog::debug("BBBBBB");
    if (this == &other)
      return *this;
    if (_sockfd != -1)
      close(_sockfd);
    _sockfd = other._sockfd;
    other._sockfd = -1;
    return *this;
  }

  ~Socket() {
    spdlog::debug("Socket destructor called sockfd={}", _sockfd);
    if (_sockfd != -1) {
      spdlog::debug("Closing sockfd={}", _sockfd);
      close(_sockfd);
    }
  }
};

} // namespace toad
