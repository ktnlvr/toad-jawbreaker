#pragma once

#include <coroutine>
#include <exception>
#include <optional>

#include "executor.hpp"

namespace toad {

struct Task;

void spawn(Task &&task);

struct Task {
  struct promise_type {
    using coroutine_handle = std::coroutine_handle<promise_type>;

    std::exception_ptr exception;
    coroutine_handle continuation;

    /// @brief Returned when the coroutine encounters a `co_return`.
    /// The `co_return` is desugared into `co_await promise.return_void`.
    void return_void() {}

    auto get_return_object() {
      return Task(coroutine_handle::from_promise(*this));
    }

    /// @brief Called IMMEDIATELY after the coroutine is initialized
    auto initial_suspend() { return std::suspend_always{}; }

    /// @brief Called IMMEDIATELY after the coroutine is done (i.e. co_return)
    auto final_suspend() { return std::suspend_always{}; }

    void unhandled_exception() {
      // TODO: handle the exception
    }
  };

  using coroutine_handle = Task::promise_type::coroutine_handle;
  coroutine_handle handle_;

  Task() : handle_(nullptr) {}

  Task(coroutine_handle handle) : handle_(handle) {}
  Task(const Task &) = delete;

  Task(Task &&other) : handle_(other.handle_) { other.handle_ = nullptr; }
  Task &operator=(Task &&other) {
    if (this == &other)
      return *this;
    handle_ = other.handle_;
    other.handle_ = nullptr;
    return *this;
  }

  bool resume() {
    if (!handle_.done())
      handle_.resume();
    return !handle_.done();
  }

  bool done() { return handle_.done(); }

  auto operator co_await() noexcept {
    struct awaiter {
      coroutine_handle coro_;

      bool await_ready() { return coro_.done(); };
      void await_resume() {};
      void await_suspend(coroutine_handle coro) {
        // TODO: check if a continuation is already set
        coro_.promise().continuation = coro;
      };
    };

    return awaiter{handle_};
  }

  /// @brief Assume that the coroutine will be magically rescheduled elsewhere.
  /// Needed because C++ does not provide a direct ownerships mechanism, so when
  /// something except the executor takes over the coro should not be destroyed.
  void leak() { handle_ = nullptr; }

  ~Task() {
    if (handle_)
      handle_.destroy();
  }
};

} // namespace toad
