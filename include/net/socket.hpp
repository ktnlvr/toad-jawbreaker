#pragma once

namespace toad {

struct Socket {
  int _sockfd;

  Socket(int sockfd) : _sockfd(sockfd) {}

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  Socket(Socket &&other) : _sockfd(other._sockfd) { other._sockfd = 0; }
  Socket &operator=(Socket &&other) {
    if (this == &other)
      return *this;
    _sockfd = other._sockfd;
    other._sockfd = 0;
    return *this;
  }

  ~Socket() {
    if (_sockfd)
      close(_sockfd);
  }
};

} // namespace toad
