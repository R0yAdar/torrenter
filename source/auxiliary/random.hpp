#pragma once

#include <random>

template<typename T, T Min, T Max>
T generate_random_in_range()
{
  static std::mt19937 rng {std::random_device {}()};
  static std::uniform_int_distribution<T> uni {Min, Max};
  return uni(rng);
}

template<typename T>
T generate_random_in_range(T min, T max)
{
  static std::mt19937 rng {std::random_device {}()};
  std::uniform_int_distribution<T> uni {min, max};
  return uni(rng);
}