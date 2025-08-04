#pragma once

#include <atomic>
#include <condition_variable>

namespace toad {

void spawn(std::coroutine_handle<>);

struct Notify {
  struct Awaiter {
    Notify &notify;
    Awaiter *next = nullptr;
    std::coroutine_handle<> continuation = nullptr;

    bool await_ready() { return false; };
    void await_resume() {};
    void await_suspend(std::coroutine_handle<> handle) {
      continuation = handle;
      notify.push_awaiter_(this);
    };

    Awaiter(Notify &notify) : notify(notify) {}
  };

  std::atomic<Awaiter *> head;

  Notify() : head(nullptr) {}

  Notify(const Notify &other) = delete;
  Notify(Notify &&other) = delete;

  void notify_all() {
    auto current = head.exchange(nullptr, std::memory_order_acquire);

    while (current) {
      spawn(current->continuation);
      current = current->next;
    }
  }

  void wait_blocking() {
    /// XXX(Artur): possible race condition. Entirely feasable for this to be
    /// called before any awaiters have been registered or after they all have
    /// been notified. Need to fix that.
    while (head.load(std::memory_order_acquire) != nullptr)
      std::this_thread::yield();
  }

  void push_awaiter_(Awaiter *awaiter) {
    Awaiter *old_head = head.load(std::memory_order_relaxed);

    do {
      awaiter->next = old_head;
    } while (!head.compare_exchange_weak(old_head, awaiter,
                                         std::memory_order_release,
                                         std::memory_order_relaxed));
  }

  auto operator co_await() noexcept { return Awaiter(*this); }
};

} // namespace toad