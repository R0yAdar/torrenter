#include <algorithm>
#include <array>
#include <iostream>
#include <string_view>

#include "client/peer.hpp"

#include <boost/asio.hpp>

#include "client/transmit/boost_transmit.hpp"
#include "torrent/messages.hpp"

using boost::asio::ip::address;
using boost::asio::ip::port_type;

namespace btr
{

Peer::Peer(std::shared_ptr<const InternalContext> context,
           address address,
           port_type port)
    : m_address {address}
    , m_port {port}
    , m_application_context {context}
    , m_context {}
{
}

std::weak_ptr<const ExternalPeerContext> Peer::get_context()
{
  return {m_context};
}

boost::asio::awaitable<void> Peer::connect_async(boost::asio::io_context& io)
{
  try {
    boost::asio::ip::tcp::socket socket(io);

    co_await socket.async_connect(
        boost::asio::ip::tcp::endpoint(m_address, m_port),
        boost::asio::use_awaitable);
    {
      btr::Handshake handshake {*m_application_context};

      co_await send_handshake(socket, handshake);
    }

    auto handshake = co_await read_handshake(socket);

    std::string_view peer_id(handshake.peer_id, 10);

    std::cout << '<' << m_address << "> PeerId: <" << peer_id << '>'
              << std::endl;

    auto message = (co_await read_message(socket));

    if (message) {
      std::cout << "Read message, type: " << message->index() << std::endl;
    }

    socket.close();
  } catch (const std::exception& e) {
  }
}
}  // namespace btr