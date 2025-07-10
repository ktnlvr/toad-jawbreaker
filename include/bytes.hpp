#pragma once

#include <optional>

#include "defs.hpp"

namespace toad {

struct Bytes {
  u8 *ptr;
  sz size, cursor;

  Bytes(u8 *ptr, sz size) : ptr(ptr), size(size), cursor(0) {}

  auto snapshot() -> sz { return cursor; }

  void recover(sz snapshot) { cursor = snapshot; }

  bool write_u8(u8 value) {
    if (size - cursor == 1)
      return false;
    ptr[cursor++] = value;
    return true;
  }

  bool write_u16(u16 value) {
    if (size - cursor == 1)
      return false;

    if constexpr (std::endian::native == std::endian::big)
      value = std::byteswap(value);

    ptr[cursor++] = (value >> 8) & 0xFF;
    ptr[cursor++] = value & 0xFF;
    return true;
  }

  bool write_array(const u8 *ptr, sz size) {
    if (size > remaining())
      return false;
    std::memcpy(this->ptr + cursor, ptr, size);
    cursor += size;
    return true;
  }

  bool write_rest(const u8 *ptr, sz size) {
    size = std::min(size, remaining());
    std::memcpy(this->ptr + cursor, ptr, size);
    cursor += size;
    return true;
  }

  auto read_u8() -> std::optional<u8> {
    if (cursor == 0)
      return std::nullopt;

    return ptr[cursor++];
  }

  auto read_u16() -> std::optional<u16> {
    if (size - cursor <= 1)
      return std::nullopt;

    u8 a = ptr[cursor++];
    u8 b = ptr[cursor++];
    u16 value = (u16(a) << 8) | b;

    if constexpr (std::endian::native == std::endian::big)
      value = std::byteswap(value);

    return value;
  }

  sz remaining() const { return size - cursor; }

  void read_rest(u8 *ptr) {
    std::memcpy(ptr, this->ptr + cursor, remaining());
    cursor = size;
  }

  void read_array(u8 *ptr, sz size) {
    size = std::min(size, remaining());
    std::memcpy(ptr, this->ptr, size);
    cursor += size;
  }
};

} // namespace toad