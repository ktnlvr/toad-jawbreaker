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

Task producer(Notify &notify, FutureHandle<int> handle) {
  spdlog::info("Producer Before notify");
  notify.notify_all();
  spdlog::info("Producer After notify");
  handle.set_value(42);
  co_return;
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

  Notify notify;
  auto [fut, handle] = Future<int>::make_future();

  executor.spawn(worker1(notify));
  executor.spawn(worker2(notify));
  executor.spawn(producer(notify, handle));
  executor.spawn(consumer(std::move(fut), notify));

  notify.wait_blocking();

  return 0;
}