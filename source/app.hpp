#pragma once

#include <filesystem>
#include <string>

#include "client/storage/storage.hpp"

class App
{
  std::shared_ptr<IStorage> m_piece_vault;

public:
  App(std::filesystem::path piece_vault_root);

  void Run(const std::string& torrent_file_path,
           const std::string& download_path);
};
