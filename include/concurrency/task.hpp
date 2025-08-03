#pragma once

#include <coroutine>
#include <exception>
#include <optional>

#include "executor.hpp"

namespace toad {

template <typename T> struct Task {
  struct promise_type {
    using Handle = std::coroutine_handle<promise_type>;

    std::optional<T> result;
    std::exception_ptr exception;
    std::coroutine_handle<> continuation;

    Task get_return_object() { return Task(Handle::from_promise(*this)); }

    std::suspend_always initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept {
      struct FinalAwaiter : std::suspend_always {
        void await_suspend(promise_type::Handle h) noexcept {
          if (h.promise().continuation)
            h.promise().continuation.resume();
        }
      };
      return FinalAwaiter{};
    }

    void return_value(T value) noexcept { result = std::move(value); }

    void unhandled_exception() noexcept {
      exception = std::current_exception();
    }
  };

  explicit Task(typename promise_type::Handle handle) : _handle(handle) {}

  Task(const Task &task) = delete;
  Task &operator=(const Task &) = delete;

  Task(Task &&other) noexcept : _handle(other._handle) { other._handle = {}; }

  Task &operator=(Task &&other) noexcept {
    if (this == &other)
      return *this;
    if (_handle && !_handle.done())
      _handle.destroy();
    _handle = other._handle;
    other._handle = {};
    return *this;
  }

  ~Task() {
    if (_handle)
      _handle.destroy();
  }

  auto operator co_await() & = delete;

  auto operator co_await() && noexcept {
    struct Awaiter {
      typename promise_type::Handle _handle;

      bool await_ready() noexcept { return false; }

      bool await_suspend(std::coroutine_handle<> handle) noexcept {
        if (!_handle || _handle.done())
          return false;

        _handle.promise().continuation = handle;
        spawn(_handle);
        return true;
      }

      T await_resume() {
        if (_handle.promise().exception)
          std::rethrow_exception(_handle.promise().exception);
        T tmp = *std::move(_handle.promise().result);
        _handle.promise().result.reset();
        return tmp;
      }

      ~Awaiter() {
        if (_handle)
          _handle.destroy();
      }
    };

    return Awaiter{std::exchange(_handle, nullptr)};
  }

  T get() {
    if (_handle.promise().exception)
      std::rethrow_exception(_handle.promise().exception);
    auto tmp = *std::move(_handle.promise().result);
    _handle.promise().result.reset();
    return tmp;
  }

  operator ErasedHandle() && {
    auto tmp = std::exchange(_handle, nullptr);
    return ErasedHandle(tmp);
  }

  typename promise_type::Handle _handle;
};

template <> struct Task<void> {
  struct promise_type {
    using Handle = std::coroutine_handle<promise_type>;
    std::exception_ptr exception;
    std::coroutine_handle<> continuation;

    Task get_return_object() { return Task{Handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept {
      struct FinalAwaiter : std::suspend_always {
        void await_suspend(promise_type::Handle h) noexcept {
          if (h.promise().continuation)
            h.promise().continuation.resume();
        }
      };
      return FinalAwaiter{};
    }

    void return_void() noexcept {}

    void unhandled_exception() noexcept {
      exception = std::current_exception();
    }
  };

  promise_type::Handle _handle;

  using Handle = typename promise_type::Handle;

  explicit Task(Handle _handle) : _handle(_handle) {}
  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;

  Task(Task &&o) noexcept : _handle(o._handle) { o._handle = nullptr; }
  Task &operator=(Task &&other) noexcept {
    if (this == &other)
      return *this;
    if (_handle && !_handle.done())
      _handle.destroy();
    _handle = other._handle;
    other._handle = {};
    return *this;
  }

  ~Task() {
    if (_handle)
      _handle.destroy();
  }

  auto operator co_await() & = delete;

  auto operator co_await() && noexcept {
    struct Awaiter {
      Handle _handle;
      bool await_ready() noexcept { return false; }

      bool await_suspend(std::coroutine_handle<> handle) noexcept {
        if (!_handle || _handle.done())
          return false;

        _handle.promise().continuation = handle;
        spawn(_handle);
        return true;
      }

      void await_resume() {
        if (_handle.promise().exception)
          std::rethrow_exception(_handle.promise().exception);
      }

      ~Awaiter() {
        if (_handle)
          _handle.destroy();
      }
    };
    return Awaiter{std::exchange(_handle, nullptr)};
  }

  void get() {
    if (_handle.promise().exception)
      std::rethrow_exception(_handle.promise().exception);
  }

  operator ErasedHandle() && {
    return ErasedHandle(std::exchange(_handle, nullptr));
  }
};

template <typename T> using Handle = typename Task<T>::promise_type::Handle;

} // namespace toad
