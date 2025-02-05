#include <algorithm>
#include <iostream>

#include <boost/asio.hpp>

#include "torrent/messages.hpp"
#include "client/peer.hpp"

using boost::asio::ip::address;
using boost::asio::ip::port_type;

namespace btr
{

Peer::Peer(std::shared_ptr<const PeerContext> context,
     address address,
     port_type port)
    : m_address {address}
    , m_port {port}
    , m_context {context}
{
}

boost::asio::awaitable<void> Peer::connect_async(boost::asio::io_context& io)
{
  boost::asio::ip::tcp::socket socket(io);

  try {
    co_await socket.async_connect(
        boost::asio::ip::tcp::endpoint(m_address, m_port),
        boost::asio::use_awaitable);

    std::cout << "Connected to " << m_address << std::endl;

    btr::Handshake handshake {*m_context};

    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(&handshake, sizeof(handshake)),
        boost::asio::use_awaitable);

    socket.close();
  } catch (const std::exception& e) {
    std::cout << "Failed to connect to " << m_address << std::endl;
  }
}
}  // namespace btr