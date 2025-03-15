#include <iostream>
#include <string>

#include "app.hpp"

auto main() -> int
{
  try {
    auto app = App {std::filesystem::path {"C:\\torrents"}};

    app.Run("C:\\Users\\royad\\Downloads\\peak.torrent",
            "C:\\Users\\royad\\Downloads\\peak.mp4");

  } catch (std::invalid_argument& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
  }
  return 0;
}
