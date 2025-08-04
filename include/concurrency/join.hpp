#pragma once

#include <array>

#include "executor.hpp"
#include "notify.hpp"
#include "task.hpp"

namespace toad {

template <typename... Tasks> void join_blocking(Tasks... tasks) {
  constexpr sz count = sizeof...(tasks);
  std::array<Task, count> array = {std::move(tasks)...};
  std::array<Notify *, count> notifies;

  std::mutex mu_;
  std::condition_variable cv_;
  std::atomic<u32> remaining = count;
  for (sz i = 0; i < count; i++) {
    auto *notify = &array[i].notify_when_done();
    spawn([notify, &cv_, &remaining]() -> Task {
      co_await *notify;
      spdlog::info("NOTIFIED!!!");
      if (remaining.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        cv_.notify_one();
      }
    }());
  }

  for (sz i = 0; i < count; i++) {
    spawn(std::move(array[i]));
  }

  std::unique_lock lock_(mu_);
  cv_.wait(lock_, [&remaining]() -> bool {
    return remaining.load(std::memory_order_seq_cst) == 0;
  });

  spdlog::info("Join done!");
}

} // namespace toad