#include "concurrency/executor.hpp"
#include "concurrency/future.hpp"
#include "concurrency/join.hpp"
#include "concurrency/notify.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

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
  for (int i = 0; i < 5; i++) {
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
  auto formatter = std::make_unique<spdlog::pattern_formatter>();
  formatter->add_flag<CorrelationIdFormatter>('Z');
  formatter->set_pattern("%Y-%m-%d %H:%M:%S.%e [%L] [cid=%Z] [thread %t] %v");

  auto logger = spdlog::stdout_color_mt("console");
  logger->set_formatter(std::move(formatter));
  logger->set_level(spdlog::level::trace);
  spdlog::set_default_logger(logger);

  Executor executor(1);

  {
    JoinSet join_set_;

    auto worker = long_worker();
    Notify &notify = worker.notify_when_done();

    join_set_.spawn(std::move(worker));
    join_set_.spawn(worker1(notify));
    join_set_.spawn(worker2(notify));

    join_set_.wait_blocking();
  }

  return 0;
}