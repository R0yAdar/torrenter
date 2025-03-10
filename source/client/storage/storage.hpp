#pragma once

#include <string_view>
#include <filesystem>

#include <boost/asio.hpp>

class IStorage
{
public:
  boost::asio::awaitable<void> virtual push_piece(std::string_view info_hash,
                                          size_t index,
                                          const std::vector<uint8_t>& data) = 0;

  boost::asio::awaitable<bool> virtual pull_piece(std::string_view info_hash,
                                          size_t index,
                                          size_t offset,
                                          std::vector<uint8_t>& buffer) = 0;

  bool virtual exists(std::string_view info_hash, size_t index) = 0;
};

class FileDirectoryStorage : public IStorage
{
  const std::string SUFFIX = ".tp";

  std::filesystem::path m_vault;

public:
  FileDirectoryStorage(std::filesystem::path vault);

  boost::asio::awaitable<void> push_piece(
      std::string_view info_hash,
      size_t index,
      const std::vector<uint8_t>& data) override final;

  boost::asio::awaitable<bool> pull_piece(
      std::string_view info_hash,
      size_t index,
      size_t offset,
      std::vector<uint8_t>& buffer) override final;

  bool exists(std::string_view info_hash, size_t index) override final;
};