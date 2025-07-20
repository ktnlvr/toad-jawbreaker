#pragma once

#include "buffer.hpp"

namespace toad {

enum struct ByteStreamErrorCode {
  Ok,
  NotEnoughData,
};

struct ByteIStream {
  Buffer buffer;
  sz cursor, last_size;
  ByteStreamErrorCode errc;

  ByteIStream(const Buffer &buffer)
      : buffer(buffer), errc(ByteStreamErrorCode::Ok), cursor(0), last_size(0) {
  }

  sz remaining() { return buffer.size - cursor; }

  auto read_u8(u8 *out) -> ByteIStream & {
    if (cursor + 1 <= buffer.size) {
      *out = buffer.data()[cursor++];
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
      last_size = 1;
    }

    return *this;
  }

  auto read_u16(u16 *out) -> ByteIStream & {
    if (cursor + 2 <= buffer.size) {
      u16 hi = buffer.data()[cursor++];
      u16 lo = buffer.data()[cursor++];
      *out = (hi << 8) | lo;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
      last_size = 2;
    }

    return *this;
  }

  template <sz size>
  auto read_array(std::array<u8, size> *out) -> ByteIStream & {
    if (cursor + size <= buffer.size) {
      std::memcpy(out->data(), buffer.data() + cursor, size);
      cursor += size;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
      last_size = size;
    }

    return *this;
  }

  auto read_vector(std::vector<u8> *vec, std::optional<sz> count = std::nullopt)
      -> ByteIStream & {
    sz size = count.value_or(vec->size());
    if (cursor + size <= buffer.size) {
      vec->resize(size);
      std::memcpy(vec->data(), buffer.data() + cursor, size);
      cursor += size;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
      last_size = size;
    }
    return *this;
  }

  auto read_buffer(Buffer *buffer) -> ByteIStream {
    *buffer = this->buffer.slice(cursor, this->buffer.size);
    return *this;
  }
};

struct ByteOStream {
  Buffer buffer;
  sz cursor;
  ByteStreamErrorCode errc;

  ByteOStream(const Buffer &buffer)
      : buffer(buffer), errc(ByteStreamErrorCode::Ok), cursor(0) {}

  auto write_u8(u8 value) -> ByteOStream & {
    if (cursor + 1 <= buffer.size) {
      buffer.data()[cursor++] = value;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }

    return *this;
  }

  auto write_u16(u16 value) -> ByteOStream & {
    if (cursor + 2 <= buffer.size) {
      buffer.data()[cursor++] = (value >> 8) & 0xFF;
      buffer.data()[cursor++] = value & 0xFF;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }

    return *this;
  }

  template <sz size>
  auto write_array(const std::array<u8, size> &arr) -> ByteOStream & {
    if (cursor + arr.size() <= buffer.size) {
      std::memcpy(buffer.data() + cursor, arr.data(), size);
      cursor += size;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }

    return *this;
  }

  auto write_vector(const std::vector<u8> &vec) -> ByteOStream & {
    if (cursor + vec.size() <= buffer.size) {
      std::memcpy(buffer.data() + cursor, vec.data(), vec.size());
      cursor += vec.size();
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }
    return *this;
  }

  auto write_buffer(const Buffer &buf) -> ByteOStream & {
    if (cursor + buf.size <= buffer.size) {
      std::memcpy(buffer.data() + cursor, buf.data(), buf.size);
      cursor += buf.size;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }
    return *this;
  }
};

} // namespace toad
