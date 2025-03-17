#pragma once

#include <expected>

#include <boost/asio.hpp>

#include "torrent/messages.hpp"

using boost::asio::awaitable;
using boost::asio::ip::tcp;

namespace btr
{
awaitable<void> send_message(tcp::socket& socket, TorrentMessage& message);

enum class ParseError
{
  LengthMismatch,
  UnknownId,
};

awaitable<std::expected<TorrentMessage, ParseError>> read_message(
    tcp::socket& socket);

awaitable<Handshake> read_handshake(tcp::socket& socket);

awaitable<void> send_handshake(tcp::socket& socket, Handshake handshake);
}  // namespace btr