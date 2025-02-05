#pragma once
#include <algorithm>
#include <expected>
#include <iostream>
#include <vector>

#include <boost/asio.hpp>

#include "torrent/tracker_messages.hpp"
#include "client/torrenter.hpp"

using boost::asio::ip::address;
using boost::asio::ip::port_type;
using boost::asio::ip::udp;
using boost::system::error_code;
using namespace boost::asio;

namespace btr
{
std::expected<std::vector<udp::endpoint>, error_code> udp_fetch_swarm(
    const Torrenter& torrenter, address address, port_type port);
}  // namespace btr
