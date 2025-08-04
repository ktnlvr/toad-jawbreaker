#pragma once

#include <array>
#include <latch>

#include "executor.hpp"
#include "notify.hpp"
#include "task.hpp"

namespace toad {

template <typename... Tasks> void join_blocking(Tasks... tasks) {
  constexpr sz count = sizeof...(tasks);
  std::array<Task, count> array = {std::move(tasks)...};

  std::latch done(count);
  for (sz i = 0; i < count; i++) {
    auto *notify = &array[i].notify_when_done();
    spawn([notify, &done]() -> Task {
      co_await *notify;
      done.count_down();
      spdlog::info("NOTIFIED!!!");
    }());
  }

  for (sz i = 0; i < count; i++)
    spawn(std::move(array[i]));

  done.wait();
  spdlog::info("Join done!");
}

} // namespace toad