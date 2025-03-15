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
using namespace boost::asio;
using namespace std::chrono_literals;

namespace btr
{
class Tracker
{
  std::shared_ptr<InternalContext> m_context;
  address m_address;
  port_type m_port;

public:
  Tracker(std::shared_ptr<InternalContext> context,
          address address,
          port_type port);

  boost::asio::awaitable<void> fetch_udp_swarm(
      boost::asio::any_io_executor& io,
      std::vector<udp::endpoint>& endpoints,
      std::chrono::seconds timeout = 1s) const;
};
}  // namespace btr
