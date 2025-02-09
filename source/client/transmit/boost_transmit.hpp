#pragma once

#include <expected>

#include <boost/asio.hpp>

#include "torrent/messages.hpp"

namespace btr
{
template<uint32_t AnyLength, uint8_t AnyId>
boost::asio::awaitable<void> send_message(
    boost::asio::ip::tcp::socket& socket,
    MessageMetadata<AnyLength, AnyId> message);

boost::asio::awaitable<void> send_message(boost::asio::ip::tcp::socket& socket,
                                          Keepalive message);

template<SupportsDynamicLength Metadata>
boost::asio::awaitable<void> send_message(
    boost::asio::ip::tcp::socket& socket,
    DynamicLengthMessage<Metadata> message);

enum class ParseError
{
  LengthMismatch,
  UnknownId,
};

boost::asio::awaitable<std::expected<TorrentMessage, ParseError>> read_message(
    boost::asio::ip::tcp::socket& socket);

boost::asio::awaitable<Handshake> read_handshake(
    boost::asio::ip::tcp::socket& socket);

boost::asio::awaitable<void> send_handshake(
    boost::asio::ip::tcp::socket& socket, Handshake handshake);
}  // namespace btr