#pragma once

#include <coroutine>

namespace toad {

struct Task {
  struct promise_type {
    using Handle = std::coroutine_handle<promise_type>;

    Task get_return_object() { return Task(Handle::from_promise(*this)); }

    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  explicit Task(promise_type::Handle handle) : _handle(handle) {}

  Task(const Task &task) = delete;
  Task &operator=(const Task &) = delete;

  Task(Task &&other) : _handle(other._handle) { other._handle = {}; }
  Task &operator=(Task &&other) noexcept {
    if (this == &other)
      return *this;
    if (_handle)
      _handle.destroy();
    _handle = other._handle;
    other._handle = {};
    return *this;
  }

  ~Task() {
    if (_handle && !_handle.done())
      _handle.destroy();
  }

  promise_type::Handle _handle;
};

using Handle = Task::promise_type::Handle;

} // namespace toad
