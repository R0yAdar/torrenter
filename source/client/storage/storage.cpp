#include <print>

#include "storage.hpp"

#include <boost/asio.hpp>
#include <boost/asio/stream_file.hpp>

FileDirectoryStorage::FileDirectoryStorage(std::filesystem::path vault)
    : m_vault {std::move(vault)}
{
}

/// TODO: make this return bool
boost::asio::awaitable<void> FileDirectoryStorage::push_piece(
    std::string_view info_hash, size_t index, const std::vector<uint8_t>& data)
{
  auto path = m_vault / info_hash;

  if (!std::filesystem::exists(path)) {
    std::filesystem::create_directories(path);
  }

  path /= (std::to_string(index) + SUFFIX);

  if (std::filesystem::exists(path)) {
    co_return;
  }

  boost::asio::stream_file file {co_await boost::asio::this_coro::executor,
                                 path.string(),
                                 boost::asio::stream_file::flags::write_only
                                     | boost::asio::stream_file::flags::create};
  co_await boost::asio::async_write(
      file, boost::asio::buffer(data), boost::asio::use_awaitable);
}

boost::asio::awaitable<bool> FileDirectoryStorage::pull_piece(
    std::string_view info_hash,
    size_t index,
    size_t offset,
    size_t amount,
    std::vector<uint8_t>& buffer)
{
  auto path = m_vault / info_hash / (std::to_string(index) + SUFFIX);

  if ((!std::filesystem::exists(path)) || (buffer.size() < amount)) {
    co_return false;
  }

  boost::asio::random_access_file file {
      co_await boost::asio::this_coro::executor,
      path.string(),
      boost::asio::stream_file::flags::read_only};

  co_await boost::asio::async_read_at(
      file, offset, boost::asio::buffer(buffer, amount), boost::asio::use_awaitable);

  co_return true;
}

bool FileDirectoryStorage::exists(std::string_view info_hash, size_t index)
{
  auto path = m_vault / info_hash / (std::to_string(index) + SUFFIX);

  return std::filesystem::exists(path);
}

