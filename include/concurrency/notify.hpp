#pragma once

#include <atomic>
#include <condition_variable>

namespace toad {

void spawn(std::coroutine_handle<>);

struct Notify {
  struct Awaiter {
    Notify &notify_;
    Awaiter *next = nullptr;
    std::coroutine_handle<> continuation = nullptr;

    bool await_ready() noexcept {
      return notify_.fired_.load(std::memory_order_acquire);
    };

    void await_resume() {};
    void await_suspend(std::coroutine_handle<> handle) {
      continuation = handle;
      notify_.push_awaiter_(this);
    };

    Awaiter(Notify &notify) : notify_(notify) {}
  };

  std::atomic<Awaiter *> head_;
  std::atomic<bool> fired_{false};

  Notify() : head_(nullptr), fired_(false) {}

  Notify(const Notify &other) = delete;
  Notify(Notify &&other) = delete;

  void notify_all() {
    fired_.store(true, std::memory_order_release);
    auto current = head_.exchange(nullptr, std::memory_order_acquire);

    while (current) {
      spawn(current->continuation);
      current = current->next;
    }
  }

  // TODO: notify one

  void wait_blocking() {
    while (!fired_.load(std::memory_order_acquire))
      std::this_thread::yield();
  }

  void push_awaiter_(Awaiter *awaiter) {
    Awaiter *old_head = head_.load(std::memory_order_relaxed);

    do {
      awaiter->next = old_head;
    } while (!head_.compare_exchange_weak(old_head, awaiter,
                                          std::memory_order_release,
                                          std::memory_order_relaxed));
  }

  auto operator co_await() noexcept { return Awaiter(*this); }
};

} // namespace toad