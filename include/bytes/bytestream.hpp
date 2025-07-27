#pragma once

#include "buffer.hpp"

namespace toad {

enum struct ByteStreamErrorCode {
  Ok,
  NotEnoughData,
};

template <typename B>
concept ByteBuffer = requires(B buffer, sz idx) {
  { buffer.size() } -> std::same_as<sz>;
  { buffer.data() } -> std::same_as<u8 *>;
  { buffer[idx] } -> std::same_as<u8 &>;
};

// NOTE(Artur): one might think that this works with a forward iterator, but
// this is a convenience structure for interacting with an underlying buffer
// that is being dynammicaly pushed into. Hence, if it was to use a
// forward-iterator it would get invalidated all the time.
template <ByteBuffer B> struct ByteIStream {
  B &buffer;
  sz cursor, last_size;
  ByteStreamErrorCode errc;

  ByteIStream(B &buffer)
      : buffer(buffer), errc(ByteStreamErrorCode::Ok), cursor(0), last_size(0) {
  }

  sz remaining() { return buffer.size() - cursor; }

  auto read_u8(u8 *out) -> ByteIStream & {
    if (cursor + 1 <= buffer.size()) {
      *out = buffer.data()[cursor++];
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
      last_size = 1;
    }

    return *this;
  }

  auto read_u16(u16 *out) -> ByteIStream & {
    if (cursor + 2 <= buffer.size()) {
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
    if (cursor + size <= buffer.size()) {
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
    if (cursor + size <= buffer.size()) {
      vec->resize(size);
      std::memcpy(vec->data(), buffer.data() + cursor, size);
      cursor += size;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
      last_size = size;
    }
    return *this;
  }

  [[deprecated("No implementation currently exists. Needs to be "
               "filled in.")]]
  auto read_buffer(Buffer *buffer) -> ByteIStream & {
    ASSERT(false, ":C");
    return *this;
  }
};

template <ByteBuffer B> struct ByteOStream {
  B &buffer;
  sz cursor;
  ByteStreamErrorCode errc;

  ByteOStream(B &buffer)
      : buffer(buffer), errc(ByteStreamErrorCode::Ok), cursor(0) {}

  auto write_u8(u8 value) -> ByteOStream & {
    if (cursor + 1 <= buffer.size()) {
      buffer.data()[cursor++] = value;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }

    return *this;
  }

  auto write_u16(u16 value) -> ByteOStream & {
    if (cursor + 2 <= buffer.size()) {
      buffer.data()[cursor++] = (value >> 8) & 0xFF;
      buffer.data()[cursor++] = value & 0xFF;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }

    return *this;
  }

  template <sz size>
  auto write_array(const std::array<u8, size> &arr) -> ByteOStream & {
    if (cursor + arr.size() <= buffer.size()) {
      std::memcpy(buffer.data() + cursor, arr.data(), size);
      cursor += size;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }

    return *this;
  }

  auto write_vector(const std::vector<u8> &vec) -> ByteOStream & {
    if (cursor + vec.size() <= buffer.size()) {
      std::memcpy(buffer.data() + cursor, vec.data(), vec.size());
      cursor += vec.size();
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }
    return *this;
  }

  auto write_buffer(const Buffer &buf) -> ByteOStream & {
    if (cursor + buf._size <= buffer.size()) {
      std::memcpy(buffer.data() + cursor, buf.data(), buf._size);
      cursor += buf._size;
    } else {
      errc = ByteStreamErrorCode::NotEnoughData;
    }
    return *this;
  }
};

} // namespace toad
