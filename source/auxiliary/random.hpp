#pragma once
#include <random>

template<typename T, T Min, T Max>
T generate_random_in_range()
{
  static std::mt19937 rng {std::random_device {}()};
  static std::uniform_int_distribution<T> uni {Min, Max};
  return uni(rng);
}