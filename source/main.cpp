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
    std::cout
        << "Enter path to torrent-file (you can use https://itorrents.net/, https://nyaa.si/view/2090638, etc.):\n";
    std::getline(std::cin, torrent_file_path);

    if (!std::filesystem::exists(torrent_file_path)) {
      std::cout << "Torrent file not found, exiting program.";
      return -1;
    }

    std::string download_path = torrent_file_path + ".download";

    auto app = App {std::filesystem::path {torrent_vault_path}};

    app.run(torrent_file_path, download_path);

    std::cout << "Finished downloading.\n";
  } catch (std::invalid_argument& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return -1;
  }
  return 0;
}
