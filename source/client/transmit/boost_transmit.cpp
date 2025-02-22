#include <expected>
#include <iostream>
#include <utility>

#include "client/transmit/boost_transmit.hpp"

namespace btr
{
namespace
{
template<uint32_t AnyLength, uint8_t AnyId>
awaitable<void> send_message(tcp::socket& socket,
                             MessageMetadata<AnyLength, AnyId>& message)
{
  co_await boost::asio::async_write(
      socket,
      boost::asio::buffer(&message, sizeof(message)),
      boost::asio::use_awaitable);
}
template<typename PackedStruct>
awaitable<void> send_message(tcp::socket& socket, PackedStruct& message)
{
  co_await boost::asio::async_write(
      socket,
      boost::asio::buffer(&message, sizeof(message)),
      boost::asio::use_awaitable);
}

awaitable<void> send_message(tcp::socket& socket, Keepalive& message)
{
  co_await boost::asio::async_write(
      socket,
      boost::asio::buffer(&message, sizeof(message)),
      boost::asio::use_awaitable);
}

template<SupportsDynamicLength Metadata>
awaitable<void> send_message(tcp::socket& socket,
                             DynamicLengthMessage<Metadata>& message)
{
  auto metadata = message.get_metadata();

  std::array<boost::asio::const_buffer, 2> buffers = {
      boost::asio::buffer(&metadata, sizeof(metadata)),
      boost::asio::buffer(message.get_payload())};

  co_await boost::asio::async_write(
      socket, buffers, boost::asio::use_awaitable);
}
}  // namespace

awaitable<void> send_message(tcp::socket& socket, TorrentMessage& message)
{
  switch (message.index() - 1) {
    case ID_KEEPALIVE:
      co_await send_message(socket, std::get<Keepalive>(message));
      break;

    case ID_CHOKE:
      co_await send_message(socket, std::get<Choke>(message));
      break;

    case ID_UNCHOKE:
      co_await send_message(socket, std::get<Unchoke>(message));
      break;

    case ID_INTERESTED:
      co_await send_message(socket, std::get<Interested>(message));
      break;

    case ID_NOT_INTERESTED:
      co_await send_message(socket, std::get<NotInterested>(message));
      break;

    case ID_HAVE:
      co_await send_message(socket, std::get<Have>(message));
      break;

    case ID_BITFIELD:
      co_await send_message(socket, std::get<BitField>(message));
      break;

    case ID_REQUEST:
      co_await send_message(socket, std::get<Request>(message));
      break;

    case ID_PIECE:
      co_await send_message(socket, std::get<Piece>(message));
      break;

    case ID_PORT:
      co_await send_message(socket, std::get<Port>(message));
      break;

    case ID_CANCEL:
      co_await send_message(socket, std::get<Cancel>(message));
      break;
  }
}

namespace
{
template<typename T>
std::expected<T*, ParseError> reinterpret_safely(std::vector<uint8_t>& buffer)
{
  if (sizeof(T) > buffer.size()) {
    return std::unexpected(ParseError::LengthMismatch);
  }

  return reinterpret_cast<T*>(buffer.data());
}

std::expected<TorrentMessage, ParseError> parse_message(
    uint8_t id, std::vector<uint8_t>& data)
{
  switch (id) {
    case ID_CHOKE:
      return Choke {};
    case ID_UNCHOKE:
      return Unchoke {};
    case ID_INTERESTED:
      return Interested {};
    case ID_NOT_INTERESTED:
      return NotInterested {};
    case ID_HAVE:
      return reinterpret_safely<Have>(data).transform(
          [](auto* value) { return Have {*value}; });
    case ID_BITFIELD:
      return BitField {data};
    case ID_REQUEST:
      return reinterpret_safely<Request>(data).transform(
          [](auto* value) { return Request {*value}; });
    case ID_PIECE: {
      auto metadata_size =
          sizeof(decltype(std::declval<Piece>().get_metadata()));
      return reinterpret_safely<decltype(std::declval<Piece>().get_metadata())>(
                 data)
          .transform(
              [&](auto* value)
              {
                return Piece {
                    std::vector(data.cbegin() + metadata_size, data.cend()),
                    *value};
              });
    }
    case ID_CANCEL:
      return reinterpret_safely<Cancel>(data).transform(
          [](auto value) { return Cancel {*value}; });
    case ID_PORT:
      return reinterpret_safely<Port>(data).transform(
          [](auto value) { return Port {*value}; });
    default:
      return std::unexpected(ParseError::UnknownId);
  }
}
}  // namespace

awaitable<std::expected<TorrentMessage, ParseError>> read_message(
    tcp::socket& socket)
{
  uint32_big message_length;

  co_await boost::asio::async_read(
      socket,
      boost::asio::buffer(&message_length, sizeof(message_length)),
      boost::asio::use_awaitable);

  if (message_length == 0) {
    co_return Keepalive {};
  }

  std::vector<uint8_t> buffer(message_length + sizeof(message_length));

  *reinterpret_cast<int32_t*>(buffer.data()) = message_length;

  co_await boost::asio::async_read(
      socket, boost::asio::buffer(buffer.data() + sizeof(message_length), message_length), boost::asio::use_awaitable);

  co_return parse_message(buffer[sizeof(message_length)], buffer);
}

awaitable<Handshake> read_handshake(tcp::socket& socket)
{
  Handshake handshake {};

  co_await boost::asio::async_read(
      socket,
      boost::asio::buffer(&handshake, sizeof(handshake)),
      boost::asio::use_awaitable);

  co_return handshake;
}

awaitable<void> send_handshake(tcp::socket& socket, Handshake handshake)
{
  co_await boost::asio::async_write(
      socket,
      boost::asio::buffer(&handshake, sizeof(handshake)),
      boost::asio::use_awaitable);
}

}  // namespace btr