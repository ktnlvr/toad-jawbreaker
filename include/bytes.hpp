#pragma once

#include <bit>

#include "defs.hpp"

namespace toad {

enum BytesErrorCode {
  NotEnoughData,
};

struct Bytes {
  sz size, cursor;
  u8 *ptr;

  Bytes(std::vector<u8> &vec, sz cursor = 0)
      : ptr(vec.data()), size(vec.size()), cursor(cursor) {}

  Bytes(u8 *ptr, sz size, sz cursor = 0)
      : ptr(ptr), size(size), cursor(cursor) {}

  void write_u8(u8 value) { ptr[cursor++] = value; }

  void write_u16(u16 value) {
    if constexpr (std::endian::native != std::endian::little)
      value = std::byteswap(value);
    ptr[cursor++] = (value >> 8) & 0xFF;
    ptr[cursor++] = value & 0xFF;
  }

  template <sz size> void write_array(const std::array<u8, size> arr) {
    std::memcpy(ptr + cursor, arr.data(), size);
    cursor += size;
  }

  void write_vector(const std::vector<u8> &vec) {
    std::memcpy(ptr + cursor, vec.data(), vec.size());
    cursor += vec.size();
  }

  u8 read_u8() { return ptr[cursor++]; }

  u16 read_u16() {
    u16 hi = ptr[cursor++];
    u16 lo = ptr[cursor++];
    u16 le_result = (hi << 8) | lo;
    if constexpr (std::endian::native != std::endian::little)
      return std::byteswap(le_result);
    return le_result;
  }

  template <sz size> void read_array(std::array<u8, size> &out) {
    std::memcpy(out.data(), ptr + cursor, size);
    cursor += size;
  }

  sz remaining() { return size - cursor; }

  void read_vector(std::vector<u8> &vector, sz size = 0) {
    if (size == 0)
      size = remaining();

    sz old_size = vector.size();
    vector.resize(old_size + size);
    std::memcpy(vector.data() + old_size, ptr + cursor, size);
    cursor += size;
  }
};

} // namespace toad