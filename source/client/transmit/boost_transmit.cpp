#include <expected>
#include <iostream>
#include <utility>

#include "client/transmit/boost_transmit.hpp"

namespace btr
{

template<uint32_t AnyLength, uint8_t AnyId>
boost::asio::awaitable<void> send_message(
    boost::asio::ip::tcp::socket& socket,
    MessageMetadata<AnyLength, AnyId> message)
{
  co_await boost::asio::async_write(
      socket,
      boost::asio::buffer(&message, sizeof(message)),
      boost::asio::use_awaitable);
}

boost::asio::awaitable<void> send_message(boost::asio::ip::tcp::socket& socket,
                                          Keepalive message)
{
  co_await boost::asio::async_write(
      socket,
      boost::asio::buffer(&message, sizeof(message)),
      boost::asio::use_awaitable);
}
template<SupportsDynamicLength Metadata>
boost::asio::awaitable<void> send_message(
    boost::asio::ip::tcp::socket& socket,
    DynamicLengthMessage<Metadata> message)
{
  std::array<boost::asio::const_buffer, 2> buffers = {
      boost::asio::buffer(&(message.metadata), sizeof(message.metadata)),
      boost::asio::buffer(message.get_payload())};

  co_await boost::asio::async_write(
      socket, buffers, boost::asio::use_awaitable);
}

template<typename T>
std::expected<T*, ParseError> static reinterpret_safely(
    std::vector<uint8_t>& buffer)
{
  if (sizeof(T) > buffer.size()) {
    return std::unexpected(ParseError::LengthMismatch);
  }

  return reinterpret_cast<T*>(buffer.data());
}

std::expected<TorrentMessage, ParseError> static parse_message(
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

boost::asio::awaitable<std::expected<TorrentMessage, ParseError>> read_message(
    boost::asio::ip::tcp::socket& socket)
{
  uint32_big message_length;

  co_await boost::asio::async_read(
      socket,
      boost::asio::buffer(&message_length, sizeof(message_length)),
      boost::asio::use_awaitable);

  if (message_length == 0) {
    co_return Keepalive {};
  }

  uint8_t message_id;
  co_await boost::asio::async_read(
      socket,
      boost::asio::buffer(&message_id, sizeof(message_id)),
      boost::asio::use_awaitable);

  std::vector<uint8_t> buffer(message_length - sizeof(message_id));

  co_await boost::asio::async_read(
      socket, boost::asio::buffer(buffer), boost::asio::use_awaitable);

  co_return parse_message(message_id, buffer);
}

boost::asio::awaitable<Handshake> read_handshake(
    boost::asio::ip::tcp::socket& socket)
{
  Handshake handshake {};

  co_await boost::asio::async_read(
      socket,
      boost::asio::buffer(&handshake, sizeof(handshake)),
      boost::asio::use_awaitable);

  co_return handshake;
}

boost::asio::awaitable<void> send_handshake(
    boost::asio::ip::tcp::socket& socket, Handshake handshake)
{
  co_await boost::asio::async_write(
      socket,
      boost::asio::buffer(&handshake, sizeof(handshake)),
      boost::asio::use_awaitable);
}

}  // namespace btr