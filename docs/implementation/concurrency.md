# Concurrency

Toad uses [C++20 Coroutines](https://en.cppreference.com/w/cpp/language/coroutines.html) to better use CPU-time. It uses a future-like interface for generic interactions with a light [Unified-Executors-inspired](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html) interface for IO-operation submission. 

- *multithreaded*, multiple threads, literally
- *cooperative*, operations decide when they can free up the core
- *symmetric*, values can be passed back and forth
- *stackful*, recursive suspension is allowed


Coroutines themselves are a deep and complex topic which I don't find myself qualified enough to explain them, so I highly encourage looking at [Lewiss Baker's](https://lewissbaker.github.io/2017/09/25/coroutine-theory) before using them.

## Broad Strokes

To understand the concurrency model it should be sufficient to understand the 3 moving parts: `Executor`, `IOContext` and the `Futures`.

The `Executor` is a thing that decides how the concurrent tasks will be executed. You can think of it as a task queue. You can add new tasks and different threads will come over to pick them and do them. 

When writing a coroutine keep in mind that it might be given to a different thread between the suspension points. Don't rely on it unless you are doing something really specific.

```cpp
auto before = std::this_thread::get_id()

// suspend itself into the queue
co_await exec.sleep(0);                             

auto after = std::this_thread::get_id();

// 55400 55428
spdlog::info("{} {}", before, after);
```

From that follows, that you shouldn't write thread-relient code. Do **NOT** use `std::mutex`, `std::condition_variable`, `std::yield` and others unless you know what you are doing. They will stall the thread and might fuck up the scheduling algorithm degrading performance.

If you need to communicate with another thread consider using a `Future` or a `Channel`.

## Scheduling

## IOContext

`IOContext` is the abstraction over OS's async capabilities. It's usually interacted with using the `submit_*` function family. Each function schedules the respective operation to be resolved some time in the future. The data passed is considered *radioactive* until the respective awaitable returns. If a function immediately returns the data is safe. 

### Linux

On Linux [`io_uring`](https://man7.org/linux/man-pages/man7/io_uring.7.html) is a toolkit for batching asynchronous requests to the kernel. It's a big and complex magic bean. What matters is that it can minimize the amount of syscalls since every batch is a single call. Furthermore, it also can do some magic scheduling in the kernelspace to make it run even faster.

`IOContext` has a `batch_size` and a `timeout_ms`. Both define a threshold for when the next batch should be submitted, either when `N` requests piled up or `T` units of time passed.

Internally, it runs on a single thread to reduce the amount of synchronisation overhead. Because of that the hot path for handling requests does not contain any busywork mutexes or atomics.

## Further Reading

If you have to write coroutines and awaitables, consider looking into...

- MIPT C++, Konstantin Vladimirov, [Part 1](https://www.youtube.com/watch?v=mDajl0pIUjQ) and [Part 2](https://www.youtube.com/watch?v=WZhxMwKaXmw). An explanation from a compiler designer.
- [MIPT C++, Roman Lipovsky](https://www.youtube.com/watch?v=bRthJk8pKjw). Building up to coroutines in the context of concurrency.
- [cppreference](https://en.cppreference.com/w/cpp/language/coroutines.html), although it is *nauseatingly* unhelpful.
