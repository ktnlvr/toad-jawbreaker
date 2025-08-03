#include "concurrency.hpp"

using namespace toad;

int main(void) {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%L] [thread %t] %v");

  Executor executor;

  return 0;
}