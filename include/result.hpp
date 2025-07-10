#pragma once

#include <variant>

namespace toad {

template <typename T, typename E> struct Result {
  std::variant<T, E> _inner;

  Result(T t) : _inner(t) {}
  Result(E e) : _inner(e) {}

  auto is_ok() const -> bool { return std::holds_alternative<T>(this->_inner); }
  auto is_err() const -> bool { return !is_ok(); }

  auto ok() -> T && {
    ASSERT(is_ok(), "Result value must be Ok to extract");
    return std::move(std::get<T>(this->_inner));
  }

  auto error() -> E && {
    ASSERT(!is_ok(), "Attempt to extract an error out of an Ok result");
    return std::move(std::get<E>(this->_inner));
  }
};

} // namespace toad
