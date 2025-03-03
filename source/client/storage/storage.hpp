#pragma once

#include <string_view>

#include <boost/asio.hpp>

class Storage
{
  boost::asio::awaitable<void> push_piece(std::string_view info_hash,
                                          size_t index,
                                          const std::vector<uint8_t>& data);

  boost::asio::awaitable<bool> pull_piece(std::string_view info_hash,
                                          size_t index,
                                          size_t offset,
                                          std::vector<uint8_t> buffer);
};