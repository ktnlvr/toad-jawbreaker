#pragma once

#include <any>
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

    Task get_return_object() { return Task(Handle::from_promise(*this)); }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

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
    if (_handle && !_handle.done())
      _handle.destroy();
  }

  auto operator co_await() & noexcept {
    struct Awaiter {
      typename promise_type::Handle _handle;
      bool await_ready() noexcept { return false; }

      bool await_suspend(std::coroutine_handle<> handle) noexcept {
        // TODO: schedule the task back
        spawn(ErasedHandle(handle));
        return true;
      }

      T await_resume() {
        if (_handle.promise().exception)
          std::rethrow_exception(_handle.promise().exception);
        return *std::move(_handle.promise().result);
      }
    };

    return Awaiter{_handle};
  }

  T get() {
    if (_handle.promise().exception)
      std::rethrow_exception(_handle.promise().exception);
    return *std::move(_handle.promise().result);
  }

  operator ErasedHandle() {
    auto handle = ErasedHandle(_handle);
    _handle = {};
    return handle;
  }

  typename promise_type::Handle _handle;
};

template <> struct Task<void> {
  struct promise_type {
    using Handle = std::coroutine_handle<promise_type>;
    std::exception_ptr exception;

    Task get_return_object() { return Task{Handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

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
    if (_handle && !_handle.done())
      _handle.destroy();
  }

  auto operator co_await() & noexcept {
    struct Awaiter {
      Handle _handle;
      bool await_ready() noexcept { return false; }
      void await_suspend(std::coroutine_handle<>) noexcept {}

      void await_resume() {
        if (_handle.promise().exception)
          std::rethrow_exception(_handle.promise().exception);
      }
    };
    return Awaiter{_handle};
  }

  void get() {
    if (_handle.promise().exception)
      std::rethrow_exception(_handle.promise().exception);
  }

  operator ErasedHandle() {
    auto handle = ErasedHandle(_handle);
    _handle = {}; // Transfer ownership
    return handle;
  }
};

template <typename T> using Handle = typename Task<T>::promise_type::Handle;

} // namespace toad
