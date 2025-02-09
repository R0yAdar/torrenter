#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include <boost/asio.hpp>

#include "client/context.hpp"
#include "torrent/messages.hpp"

namespace btr
{
using boost::asio::ip::address;
using boost::asio::ip::port_type;

class Peer
{
private:
  address m_address;
  port_type m_port;
  std::shared_ptr<const InternalContext> m_application_context;
  std::shared_ptr<ExternalPeerContext> m_context;

public:
  Peer(std::shared_ptr<const InternalContext> context,
       address address,
       port_type port);

  std::weak_ptr<const ExternalPeerContext> get_context();

  boost::asio::awaitable<void> connect_async(boost::asio::io_context& io);
};
}  // namespace btr