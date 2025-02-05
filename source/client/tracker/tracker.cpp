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
    const Torrenter& torrenter, address address, port_type port)
{
  std::vector<udp::endpoint> result;

  boost::asio::io_service io_service;

  udp::socket socket(io_service);

  udp::endpoint remote_endpoint = udp::endpoint(address, port);

  socket.open(remote_endpoint.protocol());

  error_code err;

  ConnectRequest request {};

  auto bytesSent = socket.send_to(
      boost::asio::buffer(&request, sizeof(request)), remote_endpoint, 0, err);

  udp::endpoint sender {};
  std::vector<uint8_t> buffer {};

  buffer.resize(UINT16_MAX);
  auto size = socket.receive_from(boost::asio::buffer(buffer), sender);

  // sender should be remote_endpoint

  if (size < sizeof(ConnectResponse)) {
    // return error
  }

  ConnectResponse* response = (ConnectResponse*)buffer.data();

  auto connection_id = response->connection_id;

  AnnounceRequest annReq {torrenter, connection_id};

  socket.send_to(
      boost::asio::buffer(&annReq, sizeof(annReq)), remote_endpoint, 0, err);

  std::fill(buffer.begin(), buffer.end(), 0);

  size = socket.receive_from(boost::asio::buffer(buffer), sender);

  AnnounceResponse* anresponse = (AnnounceResponse*)buffer.data();

  IpV4Port* ips =
      reinterpret_cast<IpV4Port*>(buffer.data() + sizeof(AnnounceResponse));

  std::cout << buffer.size() << std::endl;

  auto peers_count = std::min(
      static_cast<uint16_t>((UINT16_MAX - sizeof(AnnounceResponse))
                            / sizeof(IpV4Port)),
      static_cast<uint16_t>(anresponse->leechers + anresponse->seeders));

  for (size_t i = 0; i < peers_count; i++) {
    if (ips->ip == 0) {
      break;
    }

    result.emplace_back(ip::make_address_v4(ips->ip),
                        static_cast<uint16_t>(ips->port));
    ++ips;
  }

  socket.close();
  return result;
}
}  // namespace btr
