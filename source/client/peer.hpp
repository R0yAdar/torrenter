#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include <boost/asio.hpp>

#include "torrent/messages.hpp"
#include "client/context.hpp"

namespace btr
{
using boost::asio::ip::address;
using boost::asio::ip::port_type;

class Peer
{
private:
  address m_address;
  port_type m_port;
  std::shared_ptr<const PeerContext> m_context;

public:
  Peer(std::shared_ptr<const PeerContext> context,
       address address,
       port_type port);

  boost::asio::awaitable<void> connect_async(boost::asio::io_context& io);
};
}  // namespace btr