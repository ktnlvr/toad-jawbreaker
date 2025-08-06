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

Task subworker(int i = 0) {
  spdlog::info("Subworker hard at work...");
  co_await suspend(i);
  spdlog::info("Subworker done!");
}

Task worker3() {
  spdlog::info("Worker 3 spawning subworkers...");

  {
    JoinSet join_set_;

    for (int i = 0; i < 16; i++)
      join_set_.spawn(subworker(i + 1));

    co_await join_set_;
  }

  spdlog::info("Worker 3's subworkers done! Moving on...");

  co_return;
}

Task long_worker() {
  for (int i = 0; i < 5; i++) {
    spdlog::info("Working {}...", i);
    co_await suspend();
  }
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
    join_set_.spawn(worker3());

    join_set_.wait_blocking();
  }

  return 0;
}