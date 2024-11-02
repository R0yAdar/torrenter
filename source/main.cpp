#include <iostream>
#include <string>

#include "app.hpp"

auto main() -> int
{
  try {
    auto app = App {};
    app.Run();
  } catch (std::invalid_argument& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
  }
  return 0;
}
