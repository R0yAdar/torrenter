#include "storage.hpp"

#include <boost/asio.hpp>
#include <boost/asio/stream_file.hpp>

FileDirectoryStorage::FileDirectoryStorage(std::filesystem::path vault)
    : m_vault {std::move(vault)}
{
}

boost::asio::awaitable<void> FileDirectoryStorage::push_piece(
    std::string_view info_hash, size_t index, const std::vector<uint8_t>& data)
{
  auto path = m_vault / info_hash / std::to_string(index);

  boost::asio::stream_file file {co_await boost::asio::this_coro::executor,
                                 path.string(),
                                 boost::asio::stream_file::flags::write_only};

  co_await boost::asio::async_write(
      file, boost::asio::buffer(data), boost::asio::use_awaitable);
}

boost::asio::awaitable<bool> FileDirectoryStorage::pull_piece(
    std::string_view info_hash,
    size_t index,
    size_t offset,
    std::vector<uint8_t>& buffer)
{
  auto path = m_vault / info_hash / std::to_string(index);

  if (!std::filesystem::exists(path)) {
    co_return false;
  }

  boost::asio::stream_file file {co_await boost::asio::this_coro::executor,
                                 path.string(),
                                 boost::asio::stream_file::flags::read_only};

  co_await boost::asio::async_read_at(
      file, offset, boost::asio::buffer(buffer), boost::asio::use_awaitable);

  co_return true;
}
