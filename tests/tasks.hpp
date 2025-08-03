#include <atomic>
#include <chrono>
#include <future>
#include <gtest/gtest.h>

#include "concurrency/executor.hpp"
#include "concurrency/notify.hpp"

using namespace toad;

Task worker(Notify &notify, std::atomic<int> &before, std::atomic<int> &after) {
  before.fetch_add(1, std::memory_order_seq_cst);
  co_await notify;
  after.fetch_add(1, std::memory_order_seq_cst);
}

Task producer(Notify &notify) {
  notify.notify_all();
  co_return;
}

class TasksTest : public ::testing::TestWithParam<int> {};

TEST_P(TasksTest, NotifyAllUnblocksWorkers) {
  const int N = GetParam();

  Executor executor;
  Notify notify;

  std::atomic<int> before_notify{0};
  std::atomic<int> after_notify{0};

  for (int i = 0; i < N; ++i)
    executor.spawn(worker(notify, before_notify, after_notify));

  executor.spawn(producer(notify));
  notify.wait_blocking();

  ASSERT_EQ(before_notify.load(), N);
  ASSERT_EQ(after_notify.load(), N);
}

INSTANTIATE_TEST_SUITE_P(WorkersRange, TasksTest, ::testing::Range(1, 11));
