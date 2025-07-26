#pragma once

#include <deque>
#include <mutex>

#include "future.hpp"

namespace toad {

template <typename T> struct Ring {
  // NOTE(Artur): It was all but a lie! Not an actual ring, just a vector for
  // now. I just need this thing working.

  std::mutex _mutex;
  std::deque<T> _data;

  explicit Ring(sz capacity = 1024) {}

  auto pop() -> T {
    std::lock_guard guard(_mutex);
    T value = std::move(_data.front());
    _data.pop_front();
    return std::move(value);
  }

  auto try_pop() -> std::optional<T> {
    std::lock_guard guard(_mutex);
    if (_data.empty()) {
      return std::nullopt;
    }
    T value = std::move(_data.front());
    _data.pop_front();
    return value;
  }

  void push(const T &value) {
    std::lock_guard guard(_mutex);
    _data.push_back(value);
  }

  void emplace(T &&value) {
    std::lock_guard guard(_mutex);
    _data.emplace_back(value);
  }

  auto size() -> sz {
    std::lock_guard guard(_mutex);
    return _data.size();
  }

  ~Ring() {}
};

} // namespace toad