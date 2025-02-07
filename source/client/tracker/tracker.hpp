#pragma once
#include <expected>
#include <memory>

#include <boost/asio.hpp>

#include "client/context.hpp"
#include "client/torrenter.hpp"
#include "torrent/tracker_messages.hpp"

using boost::asio::ip::address;
using boost::asio::ip::port_type;
using boost::asio::ip::udp;
using boost::system::error_code;
using namespace boost::asio;

namespace btr
{
class Tracker
{
  address m_address;
  port_type m_port;
  std::shared_ptr<PeerContext> m_context;

public:
  Tracker(std::shared_ptr<PeerContext> context,
          address address,
          port_type port);

  boost::asio::awaitable<void> fetch_udp_swarm(
      io_context& io, std::vector<udp::endpoint>& endpoints);
};
}  // namespace btr
