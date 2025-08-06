#pragma once

#include <array>
#include <latch>

#include "executor.hpp"
#include "notify.hpp"
#include "task.hpp"

namespace toad {

/// @brief Basically an implementation of a Nursery
/// https://vorpus.org/blog/notes-on-structured-concurrency-or-go-statement-considered-harmful/
/// NOT THREAD SAFE.
struct JoinSet {
  std::atomic<u32> awaiting_ = 0;
  Notify notify = {};

  std::condition_variable cv_;
  std::mutex mu_;

  void spawn(Task task) {
    ASSERT(!notify.done(), "The join must not have been joined previously");

    awaiting_.fetch_add(1, std::memory_order_seq_cst);

    auto proc = [this](Task task) -> Task {
      auto &awaiter = task.notify_when_done();
      toad::spawn(std::move(task));
      co_await awaiter;

      if (awaiting_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        notify.notify_all();
        cv_.notify_one();
      }
    }(std::move(task));

    toad::spawn(std::move(proc));
  }

  void wait_blocking() {
    std::unique_lock lock_(mu_);
    cv_.wait(lock_, [this]() { return awaiting_ == 0; });
  }

  auto operator co_await() { return notify.operator co_await(); }

  JoinSet() {}

  ~JoinSet() {
    auto remaining = awaiting_.load(std::memory_order_seq_cst);
    ASSERT(remaining == 0,
           "JoinSet did not await all coroutines, {} remaining.", remaining);
  }

  JoinSet(const JoinSet &) = delete;
  JoinSet(JoinSet &&) = delete;
};

} // namespace toad