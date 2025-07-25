#pragma once

#include <memory>
#include <optional>

#include "defs.hpp"

namespace toad {

/// @brief Shared thread-safe lock-free buffer of bytes.
struct Buffer {
  std::shared_ptr<u8[]> _shared;
  sz _offset, size;

  Buffer() : _shared{}, _offset(0), size(0) {}

  Buffer(u8 *ptr, sz size)
      : _shared(std::shared_ptr<u8[]>(new u8[size])), size(size), _offset(0) {
    std::memcpy(_shared.get(), ptr, size);
  }

  Buffer(sz size)
      : _shared(std::shared_ptr<u8[]>(new u8[size])), size(size), _offset(0) {
    std::memset(data(), 0, size);
  }

  Buffer(const std::vector<u8> &vec, std::optional<sz> size = std::nullopt)
      : size(size.value_or(vec.size())), _offset(0) {
    _shared = std::shared_ptr<u8[]>{new u8[this->size]};
    std::memcpy(data(), vec.data(), this->size);
  }

  u8 *data() const { return _shared.get() + _offset; }

  auto slice(sz length) -> Buffer { return slice(0, length); }

  auto slice(sz from, sz to) -> Buffer {
    Buffer slice = *this;
    slice._offset = slice._offset + from;
    slice.size = to - from;
    return slice;
  }
};

} // namespace toad

namespace fmt {

template <> struct formatter<toad::Buffer> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const toad::Buffer &f, FormatContext &ctx) const {
    auto out = ctx.out();
    for (toad::sz i = 0; i < f.size; i++)
      // TODO: fix trailing whitespace
      out = format_to(out, "{:02X} ", f.data()[i]);
    return out;
  }
};

} // namespace fmt
