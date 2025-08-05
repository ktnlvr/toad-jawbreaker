#pragma once

#include <coroutine>
#include <exception>
#include <optional>

#include <spdlog/pattern_formatter.h>

#include "../prng.hpp"
#include "executor.hpp"
#include "notify.hpp"

namespace toad {

using correlation_id = u64;

thread_local correlation_id thread_parent_correlation_id_ = 0;
thread_local correlation_id thread_correlation_id_ = 0;

struct Task {
  struct promise_type {
    using coroutine_handle = std::coroutine_handle<promise_type>;

    correlation_id parent_corr_id = 0;
    correlation_id corr_id = thread_safe_random_u32();
    std::exception_ptr exception;
    Notify continuations;

    /// @brief Returned when the coroutine encounters a `co_return`.
    /// The `co_return` is desugared into `co_await promise.return_void`.
    void return_void() {}

    auto get_return_object() {
      return Task(coroutine_handle::from_promise(*this));
    }

    /// @brief Called IMMEDIATELY after the coroutine is initialized
    auto initial_suspend() { return std::suspend_always{}; }

    /// @brief Called IMMEDIATELY after the coroutine is done (i.e. co_return)
    auto final_suspend() noexcept {
      spdlog::trace("Coroutine done");
      continuations.notify_all();
      return std::suspend_always{};
    }

    void unhandled_exception() {
      // TODO: handle the exception
    }
  };

  using coroutine_handle = Task::promise_type::coroutine_handle;
  coroutine_handle handle_;

  Task() : handle_(nullptr) {}

  explicit Task(coroutine_handle handle) : handle_(handle) {}

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

  auto &promise() { return handle_.promise(); }

  bool done() { return handle_.done(); }

  auto &notify_when_done() { return promise().continuations; }

  /// @brief Assume that the coroutine will be magically rescheduled elsewhere.
  /// Needed because C++ does not provide a direct ownerships mechanism, so when
  /// something except the executor takes over the coro should not be destroyed.
  void leak() { handle_ = nullptr; }

  void set_corr_id(correlation_id id) { handle_.promise().corr_id = id; }

  void set_parent_corr_id(correlation_id id) {
    handle_.promise().parent_corr_id = id;
  }

  ~Task() {
    if (handle_)
      handle_.destroy();
  }
};

using Handle = Task::coroutine_handle;

} // namespace toad

class CorrelationIdFormatter : public spdlog::custom_flag_formatter {
public:
  void format(const spdlog::details::log_msg &msg, const std::tm &tm_time,
              spdlog::memory_buf_t &dest) override {
    fmt::format_to(std::back_inserter(dest), "{:08X}",
                   toad::thread_correlation_id_);
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<CorrelationIdFormatter>();
  }
};
