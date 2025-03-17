#include <filesystem>
#include <iostream>

#include "app.hpp"

auto main() -> int
{
  try {
    std::string torrent_vault_path;
    std::cout << "Enter path to store torrent pieces at:\n";
    std::getline(std::cin, torrent_vault_path);

    if (!std::filesystem::exists(torrent_vault_path)) {
      std::cout << "Path not found on host. Defaults to C:\\torrents\\...\n";
      torrent_vault_path = "C:\\torrents";
    }

    std::string torrent_file_path;
    std::cout << "Enter path to torrent-file:\n";
    std::getline(std::cin, torrent_file_path);

    if (!std::filesystem::exists(torrent_file_path)) {
      std::cout << "Torrent file not found, exiting program.";
      return -1;
    }

    std::string download_path;
    std::cout << "Enter path for the downloaded file:\n";
    std::getline(std::cin, download_path);

    auto app = App {std::filesystem::path {"C:\\torrents"}};

    app.run(torrent_file_path, download_path);

    std::cout << "Finished download.\n";
  } catch (std::invalid_argument& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return -1;
  }
  return 0;
}
