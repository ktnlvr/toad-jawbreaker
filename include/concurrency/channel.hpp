#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "ring.hpp"

namespace toad {

template <typename T> struct Channel {
  Ring<T> _ring;

  std::mutex mutex;
  std::atomic<u32> tx_count{1};
  std::atomic_bool rx_exists{false};
  ErasedHandle continuation{};

  explicit Channel(size_t capacity) : _ring(capacity) {}
};

template <typename T> struct recv_awaiter {
  std::shared_ptr<Channel<T>> _channel;
  std::optional<T> result;

  recv_awaiter(std::shared_ptr<Channel<T>> channel)
      : _channel(channel), result(std::nullopt) {}

  bool await_ready() {
    auto popped = _channel->_ring.try_pop();
    if (popped.has_value()) {
      result = popped;
      return true;
    }

    if (_channel->tx_count.load(std::memory_order_acquire) == 0) {
      result = std::nullopt;
      return true;
    }

    return false;
  }

  bool await_suspend(std::coroutine_handle<> handle) {
    const std::lock_guard<std::mutex> guard(_channel->mutex);

    // If the new data was pushed when we were awaiting, continue immediately
    if (_channel->_ring.size() > 0 ||
        _channel->tx_count.load(std::memory_order_acquire) == 0) {
      return false;
    }

    _channel->continuation = ErasedHandle{handle};
    return true;
  }

  std::optional<T> await_resume() {
    // Either we were resumed because someone pushed or there are no more TXs
    if (result.has_value())
      return result;
    
    auto popped = _channel->_ring.try_pop();
    if (popped.has_value())
      return popped;

    // no more senders, give up
    return std::nullopt;
  }
};

template <typename T> struct TX {
  std::shared_ptr<Channel<T>> _channel;

  TX(std::shared_ptr<Channel<T>> channel) : _channel(channel) {}

  TX() = delete;
  TX(const TX &other) = delete;
  TX &operator=(const TX &other) = delete;

  TX(TX &&other) noexcept : _channel(std::exchange(other._channel, nullptr)) {}

  TX &operator=(TX &&other) noexcept {
    if (this != &other)
      _channel = std::move(other._channel);
    return *this;
  }

  bool send(T &&value) {
    if (!_channel)
      return false;

    if (!_channel->rx_exists)
      return false;

    std::lock_guard guard(_channel->mutex);
    _channel->_ring.emplace(std::move(value));
    if (_channel->continuation)
      this_executor().spawn(std::move(_channel->continuation));
    return true;
  }

  ~TX() {
    if (_channel) {
      if (_channel->tx_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        ErasedHandle handle;

        {
          std::lock_guard guard(_channel->mutex);
          handle = std::move(_channel->continuation);
        }

        if (handle)
          this_executor().spawn(std::move(handle));
      }
    }
  }
};

template <typename T> struct RX {
  std::shared_ptr<Channel<T>> _channel;

  RX(std::shared_ptr<Channel<T>> channel) : _channel(channel) {}

  RX() = delete;

  RX(const RX &other) = delete;
  RX &operator=(const RX &) = delete;

  RX(RX &&other) noexcept = default;
  RX &operator=(RX &&other) noexcept = default;

  ~RX() {
    if (_channel) {
      _channel->rx_exists.store(false, std::memory_order_release);
    }
  }

  auto recv() { return recv_awaiter<T>(_channel); }
};

template <typename T> std::pair<TX<T>, RX<T>> channel(size_t capacity) {
  auto channel = std::make_shared<Channel<T>>(capacity);
  bool expected = false;
  if (!channel->rx_exists.compare_exchange_strong(expected, true)) {
    throw std::logic_error("Receiver already exists");
  }
  return {std::move(TX<T>(channel)), std::move(RX<T>(channel))};
}

} // namespace toad