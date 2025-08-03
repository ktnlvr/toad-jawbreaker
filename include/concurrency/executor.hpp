#pragma once

#include <deque>
#include <thread>

#include "../defs.hpp"
#include "task.hpp"

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

  void spawn(Task task) {
    void *addr = task.handle_.address();
    spdlog::debug("Spawning coroutine at address: {}", addr);

    if (task.done()) {
      spdlog::debug("Coroutine at {} already done, not spawning", addr);
      return;
    }

    {
      std::lock_guard lock(_mutex);
      _queue.emplace_back(std::move(task));
      spdlog::debug("Added coroutine at {} to queue (queue size: {})", addr,
                    _queue.size());
    }

    // Wake up one thread to go and pick the task up
    _condvar.notify_one();
  }

  // todo: spawn_blocking();

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
    _this_executor = this;

    while (true) {
      Task task;
      {
        std::unique_lock lock(_mutex);
        _condvar.wait(lock, [this] { return is_done || !_queue.empty(); });

        if (is_done && _queue.empty())
          break;
        if (_queue.empty())
          continue;

        task = std::move(_queue.front());
        _queue.pop_front();
      }

      if (!task.done()) {
        task.resume();

        // SAFETY(Artur): If the coroutine is done we know that nothing else
        // will ever reschedule it again, so we can simply own it and destroy.
        if (!task.done())
          task.leak();
      }
    }

    _this_executor = nullptr;
  }

  std::vector<std::thread> _threads;
  std::deque<Task> _queue;

  std::mutex _mutex;
  std::condition_variable _condvar;

  bool is_done = false;
};

void spawn(Task &&task) { this_executor().spawn(std::move(task)); }

} // namespace toad
