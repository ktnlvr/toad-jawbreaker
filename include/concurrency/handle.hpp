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

  void resume() const noexcept {
    if (!_handle) {
      spdlog::error("Attempting to resume null handle");
      return;
    }

    void *addr = _handle.address();
    spdlog::debug("Resuming coroutine at address: {}", addr);

    uintptr_t ptr = reinterpret_cast<uintptr_t>(addr);
    if (ptr < 0x1000 || ptr > 0x7fffffffffff) {
      spdlog::warn("Coroutine handle has suspicious address: 0x{:x}", ptr);
      return;
    }

    // Check if coroutine is already done before resuming
    if (_handle.done()) {
      spdlog::warn("Attempting to resume already done coroutine at {}", addr);
      return;
    }

    _handle.resume();
    spdlog::debug("Successfully resumed coroutine at address: {}", addr);
  }

  bool done() const noexcept {
    if (_handle.address() == nullptr)
      return true;

    try {
      return _handle.done();
    } catch (...) {
      spdlog::error("Exception in done() check for handle at {}",
                    _handle.address());
      return true; // Assume it's done if we can't check
    }
  }

  void destroy() noexcept {
    void *addr = _handle.address();
    spdlog::debug("Destroying coroutine at address: {}", addr);
    if (addr != nullptr) {
      _handle.destroy();
      _handle = {};
      spdlog::debug("Successfully destroyed coroutine at address: {}", addr);
    } else {
      spdlog::warn("Attempted to destroy null coroutine handle");
    }
  }

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