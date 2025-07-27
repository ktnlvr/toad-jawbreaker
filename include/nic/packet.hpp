#pragma once

#include "../bytes/bytestream.hpp"

namespace toad {

template <typename T, typename B>
concept Packet =
    requires(T const &obj, ByteOStream<B> ostream, ByteIStream<B> istream) {
      { const_cast<T &>(obj).try_to_stream(ostream) } -> std::same_as<void>;
      { T::try_from_stream(istream) } -> std::same_as<T>;

      { T::header_size() } -> std::same_as<sz>;
      { const_cast<T &>(obj).header_dynamic_size() } -> std::same_as<sz>;
      { const_cast<T &>(obj).payload_size() } -> std::same_as<sz>;
      { const_cast<T &>(obj).size() } -> std::same_as<sz>;
    };

} // namespace toad