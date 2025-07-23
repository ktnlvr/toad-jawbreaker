#pragma once

namespace toad {

struct Socket {
  int _sockfd;

  Socket(int sockfd) : _sockfd(sockfd) {}
};

} // namespace toad
