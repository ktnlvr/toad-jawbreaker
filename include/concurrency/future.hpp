#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <tuple>
#include <vector>

#include "executor.hpp"

namespace toad {

template <typename T> struct FutureState {
  // NOTE(Artur): avoids calling constructors or destructors
  // since the value is not initialized
  union {
    T _value;
  };

  std::atomic<bool> is_ready = false;
  std::mutex _mutex = {};
  Handle continuation = {};

  FutureState() : is_ready(false) {}

  ~FutureState() {
    if (!is_ready)
      _value.~T();
  }
};

template <typename T> struct FutureHandle {
  std::weak_ptr<FutureState<T>> _state;

  FutureHandle() {}
  FutureHandle(std::shared_ptr<FutureState<T>> state) : _state(state) {}
  ~FutureHandle() {}

  FutureHandle(const FutureHandle &other) : _state(other._state) {}
  FutureHandle operator=(const FutureHandle &other) {
    _state = other._state;
    return *this;
  }

  void set_value(T &&value) {
    auto ptr = _state.lock();
    if (ptr == nullptr)
      return;

    const std::lock_guard<std::mutex> guard(ptr->_mutex);

    if (!ptr->continuation)
      // The future was never awaited
      return;

    bool expected = false;
    bool did_set = ptr->is_ready.compare_exchange_strong(
        expected, true, std::memory_order_release, std::memory_order_relaxed);
    ASSERT(did_set, "Attempt to set already-set future");

    new (&ptr->_value) T(std::move(value));
    spawn(std::move(ptr->continuation));
  }
};

template <typename T> struct Future {
  std::shared_ptr<FutureState<T>> _state;

  Future() : _state(std::make_shared<FutureState<T>>()) {}

  Future(Future &&other) : _state(std::move(other._state)) {}
  Future &operator=(Future &&other) {
    if (&other == this)
      return *this;
    this->_state = std::move(other._state);
  }

  Future(const Future &) = delete;
  Future &operator=(const Future &) = delete;

  static auto make_future() -> std::pair<Future<T>, FutureHandle<T>> {
    auto future = Future<T>();
    auto handle = FutureHandle<T>(future._state);

    return {std::move(future), handle};
  }

  bool await_ready() noexcept { return _state->is_ready; }

  bool await_suspend(Handle handle) noexcept {
    const std::lock_guard<std::mutex> guard(_state->_mutex);
    if (_state->is_ready.load(std::memory_order_acquire)) {
      return false;
    } else {
      _state->continuation = handle;
      return true;
    }
  }

  T await_resume() {
    ASSERT(_state->is_ready, "await_resume called when future was not ready");
    auto moved_out = std::move(_state->_value);
    _state->_value.~T();
    return moved_out;
  }
};

} // namespace toad
