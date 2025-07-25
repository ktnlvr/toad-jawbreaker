#pragma once

#include <deque>
#include <functional>
#include <thread>

#include "../defs.hpp"
#include "handle.hpp"

namespace toad {

struct Executor;

thread_local Executor *_this_executor = nullptr;

auto this_executor() -> Executor & {
  ASSERT(
      _this_executor != nullptr,
      "This executor is only set in worker threads. Are you not calling this "
      "from an executor's thread?");
  return *_this_executor;
}

struct Executor {
  Executor(sz num_threads = std::thread::hardware_concurrency()) {
    _this_executor = this;
    _threads.reserve(num_threads);
    for (sz i = 0; i < num_threads; i++)
      _threads.emplace_back([this]() { this->worker_thread(); });
  }

  void spawn(ErasedHandle handle) {
    void *addr = handle._handle.address();
    spdlog::debug("Spawning coroutine at address: {}", addr);

    if (handle.done()) {
      spdlog::debug("Coroutine at {} already done, not spawning", addr);
      return;
    }

    {
      std::lock_guard lock(_mutex);
      _queue.emplace_back(std::move(handle));
      spdlog::debug("Added coroutine at {} to queue (queue size: {})", addr,
                    _queue.size());
    }

    // Wake up one thread to go and pick the task up
    _condvar.notify_one();
  }

  template <typename F> void spawn_blocking(F f) {
    {
      std::lock_guard lock(_mutex);
      _blocking_queue.emplace_back(
          std::move_only_function<void()>(std::move(f)));
    }
    _condvar.notify_one();
  }

  Executor(const Executor &) = delete;
  Executor(Executor &&) = delete;

  ~Executor() {
    {
      std::unique_lock lock_(_mutex);
      is_done = true;
    }

    _condvar.notify_all();
    for (auto &thread : _threads)
      thread.join();
  }

  void worker_thread() {
    spdlog::debug("Starting worker thread!");
    _this_executor = this;

    while (true) {
      ErasedHandle handle;
      std::move_only_function<void()> blocking_task;

      bool has_coroutine = false;
      bool has_blocking = false;

      {
        std::unique_lock lock(_mutex);
        _condvar.wait(lock, [this] {
          return is_done || !_queue.empty() || !_blocking_queue.empty();
        });

        if (is_done && _queue.empty() && _blocking_queue.empty())
          break;

        if (!_blocking_queue.empty()) {
          blocking_task = std::move(_blocking_queue.front());
          _blocking_queue.pop_front();
          has_blocking = true;
        } else if (!_queue.empty()) {
          handle = std::move(_queue.front());
          _queue.pop_front();
          has_coroutine = true;
        } else {
          continue;
        }
      }

      if (has_blocking) {
        blocking_task();
      } else if (has_coroutine && !handle.done()) {
        handle.resume();
        if (handle.done())
          handle.destroy();
      }
    }

    _this_executor = nullptr;
  }

  std::vector<std::thread> _threads;
  std::deque<ErasedHandle> _queue;
  std::deque<std::move_only_function<void()>> _blocking_queue;

  std::mutex _mutex;
  std::condition_variable _condvar;

  bool is_done = false;
};

void spawn(ErasedHandle &&handle) { this_executor().spawn(std::move(handle)); }

template <typename F> void spawn_blocking(F f) {
  this_executor().spawn_blocking(f);
}

} // namespace toad
