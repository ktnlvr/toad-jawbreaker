# Concurrency

Toad uses [C++20 Coroutines](https://en.cppreference.com/w/cpp/language/coroutines.html) to better use CPU-time. It uses a future-like interface for generic interactions with a light [Unified-Executors-inspired](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html) interface for IO-operation submission. 

- *multithreaded*, multiple threads, literally
- *cooperative*, operations decide when they can free up the core
- *symmetric*, values can be passed back and forth
- *stackful*, recursive suspension is allowed

Coroutines themselves are a deep and complex topic which I don't find myself qualified enough to explain them, so I highly encourage looking at [Lewiss Baker's](https://lewissbaker.github.io/2017/09/25/coroutine-theory) before using them.

If you have to write coroutines and awaitables, consider looking into...

- MIPT C++, Konstantin Vladimirov, [Part 1](https://www.youtube.com/watch?v=mDajl0pIUjQ) and [Part 2](https://www.youtube.com/watch?v=WZhxMwKaXmw). An explanation from a compiler designer.
- [MIPT C++, Roman Lipovsky](https://www.youtube.com/watch?v=bRthJk8pKjw). Building up to coroutines in the context of concurrency.
- [cppreference](https://en.cppreference.com/w/cpp/language/coroutines.html), although it is *nauseatingly* unhelpful.
