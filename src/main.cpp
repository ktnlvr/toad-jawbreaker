#include "concurrency/executor.hpp"
#include "concurrency/future.hpp"
#include "concurrency/notify.hpp"

using namespace toad;

Task worker1(Notify &notify) {
  spdlog::info("Worker 1! Before notify");
  co_await notify;
  spdlog::info("Worker 1! After notify");
}

Task worker2(Notify &notify) {
  spdlog::info("Worker 2! Before notify");
  co_await suspend();
  co_await notify;
  spdlog::info("Worker 2! After notify");
}

Task long_worker() {
  for (int i = 0; i < 50; i++) {
    spdlog::info("Working {}...", i);
    co_await suspend();
  }
}

Task consumer(Future<int> future, Notify &notify) {
  spdlog::info("Woke up the consumer!");
  auto consumed = co_await future;
  spdlog::info("Consumed value {}", consumed);
  co_await notify;
  spdlog::info("Awaiting for more...");
}

int main(void) {
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%L] [thread %t] %v");

  Executor executor;

  auto worker = long_worker();

  Notify &notify = worker.notify_when_done();

  executor.spawn(worker1(notify));
  executor.spawn(worker2(notify));
  executor.spawn(std::move(worker));

  notify.wait_blocking();

  return 0;
}