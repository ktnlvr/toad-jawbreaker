#include "concurrency/executor.hpp"
#include "concurrency/notify.hpp"

using namespace toad;

Task worker1(Notify &notify) {
  spdlog::info("Worker 1! Before notify");
  co_await notify;
  spdlog::info("Worker 1! After notify");
}

Task worker2(Notify &notify) {
  spdlog::info("Worker 2! Before notify");
  co_await notify;
  spdlog::info("Worker 2! After notify");
}

Task producer(Notify &notify) {
  spdlog::info("Producer Before notify");
  notify.notify_all();
  spdlog::info("Producer After notify");
  co_return;
}

int main(void) {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%L] [thread %t] %v");

  Executor executor;

  Notify notify;

  executor.spawn(worker1(notify));
  executor.spawn(worker2(notify));
  executor.spawn(producer(notify));

  notify.wait_blocking();

  return 0;
}