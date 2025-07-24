#pragma once

#include <coroutine>

namespace toad {

struct ErasedHandle {
  std::coroutine_handle<> _handle;

  ErasedHandle(std::coroutine_handle<> handle) : _handle(handle) {}

  ErasedHandle() noexcept : _handle({}) {}

  ErasedHandle(const ErasedHandle &other) = delete;
  ErasedHandle &operator=(const ErasedHandle &) = delete;

  ErasedHandle(ErasedHandle &&other) : _handle(other._handle) {
    other._handle = {};
  }

  ErasedHandle &operator=(ErasedHandle &&other) noexcept {
    if (this == &other)
      return *this;
    if (_handle && !_handle.done())
      _handle.destroy();
    _handle = other._handle;
    other._handle = {};
    return *this;
  }

  template <typename P>
  ErasedHandle(std::coroutine_handle<P> handle) noexcept
      : _handle(std::coroutine_handle<>::from_address(handle.address())) {}

  void resume() const noexcept { _handle.resume(); }

  bool done() const noexcept { return _handle.done(); }

  void destroy() noexcept { _handle.destroy(); }

  template <typename Promise>
  std::coroutine_handle<Promise> as() const noexcept {
    return std::coroutine_handle<Promise>::from_address(_handle.address());
  }

  explicit operator bool() const noexcept {
    return _handle.address() != nullptr;
  }

  ~ErasedHandle() {
    // NOTE(Artur): the coroutine is already assumed to be destroyed, so don't
    // clean up its resources
  }
};

} // namespace toad