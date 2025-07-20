#pragma once

namespace toad {

enum TypestateDirection { DirectionIn, DirectionOut };

using enum TypestateDirection;

constexpr TypestateDirection
reverse_direction(TypestateDirection dir) noexcept {
  if (dir == DirectionIn)
    return DirectionOut;
  else
    return DirectionIn;
}

consteval TypestateDirection operator~(TypestateDirection d) noexcept {
  return reverse_direction(d);
}

} // namespace toad
