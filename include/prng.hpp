#pragma once

#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "defs.hpp"

namespace toad {

std::mt19937 &get_thread_rng() {
  thread_local std::mt19937 rng(std::random_device{}()); // seeded per thread
  return rng;
}

u32 thread_safe_random_u32(u32 min, u32 max) {
  std::uniform_int_distribution<u32> dist(min, max);
  return dist(get_thread_rng());
}

u32 thread_safe_random_u32() { return thread_safe_random_u32(0, ~0); }

} // namespace toad
