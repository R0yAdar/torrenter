#pragma once

#include <filesystem>
#include <string_view>

#include <boost/asio.hpp>

class IStorage
{
public:
  boost::asio::awaitable<void> virtual push_piece(
      std::string_view info_hash,
      size_t index,
      const std::vector<uint8_t>& data,
      bool overwrite = false) = 0;

  boost::asio::awaitable<bool> virtual pull_piece(
      std::string_view info_hash,
      size_t index,
      size_t offset,
      size_t amount,
      std::vector<uint8_t>& buffer) = 0;

  bool virtual exists(std::string_view info_hash, size_t index) = 0;

  virtual ~IStorage() = default;
};

class FileDirectoryStorage : public IStorage
{
  std::filesystem::path m_vault;

public:
  FileDirectoryStorage(std::filesystem::path vault);

  boost::asio::awaitable<void> push_piece(
      std::string_view info_hash,
      size_t index,
      const std::vector<uint8_t>& data,
      bool overwrite = false) override final;

  boost::asio::awaitable<bool> pull_piece(
      std::string_view info_hash,
      size_t index,
      size_t offset,
      size_t amount,
      std::vector<uint8_t>& buffer) override final;

  bool exists(std::string_view info_hash, size_t index) override final;
};