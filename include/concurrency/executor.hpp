#pragma once

#include <deque>
#include <thread>

#include "../defs.hpp"
#include "task.hpp"

namespace toad {

struct Executor {
  Executor(sz num_threads = 1) {
    _threads.reserve(num_threads);
    for (sz i = 0; i < num_threads; i++)
      _threads.emplace_back([this]() { this->worker_thread(); });
  }

  void spawn(ErasedHandle handle) {
    if (handle.done())
      return;

    {
      std::lock_guard lock(_mutex);
      _queue.emplace_back(std::move(handle));
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
    while (true) {
      ErasedHandle handle;
      {
        std::unique_lock lock(_mutex);
        _condvar.wait(lock, [this] { return is_done || !_queue.empty(); });

        if (is_done && _queue.empty())
          break;
        if (_queue.empty())
          continue;

        handle = std::move(_queue.front());
        _queue.pop_front();
      }

      if (!handle.done())
        handle.resume();
    }
  }

  std::vector<std::thread> _threads;
  std::deque<ErasedHandle> _queue;

  std::mutex _mutex;
  std::condition_variable _condvar;

  bool is_done = false;
};

} // namespace toad
