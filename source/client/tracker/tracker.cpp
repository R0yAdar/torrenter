#include <algorithm>
#include <expected>
#include <iostream>
#include <vector>

#include "client/tracker/tracker.hpp"

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include "torrent/tracker_messages.hpp"

using boost::asio::io_context;
using boost::asio::ip::address;
using boost::asio::ip::port_type;
using boost::asio::ip::udp;
using boost::system::error_code;
using time_point = std::chrono::steady_clock::time_point;
using namespace boost::asio::experimental::awaitable_operators;
using namespace std::chrono_literals;

namespace btr
{
Tracker::Tracker(std::shared_ptr<InternalContext> context,
                 address address,
                 port_type port)
    : m_context {std::move(context)}
    , m_address {address}
    , m_port {port}
{
}

template<typename T>
concept UdpTrackerMessage = requires(const T ptr) {
  {
    ptr.action
  } -> std::convertible_to<uint32_big>;
};

template<UdpTrackerMessage T>
std::expected<T*, ErrorResponse> parse_message(uint8_t* message_ptr,
                                               size_t message_size)
{
  T* message = reinterpret_cast<T*>(message_ptr);

  if (message_size < sizeof(T)
      || message->action == static_cast<uint32_t>(Actions::Error))
  {
    auto error_message = reinterpret_cast<ErrorResponse*>(message_ptr);

    if (message_size < sizeof(ErrorResponse)
        || error_message->action != static_cast<uint32_t>(Actions::Error))
    {
      return std::unexpected(ErrorResponse {});
    }

    ErrorResponse error {};

    error.action = error_message->action;
    error.transaction_id = error_message->transaction_id;
    (*((&error_message->action) + message_size - 1)) = 0;  // null terminator
    error.message = std::string(
        ((reinterpret_cast<char*>(&(error_message->transaction_id) + 1))));
  }

  return {message};
}

std::vector<udp::endpoint> parse_ipv4_peer_endpoints(AnnounceResponse* response,
                                                     size_t max_size)
{
  std::vector<udp::endpoint> endpoints {};

  IpV4Port* ips = reinterpret_cast<IpV4Port*>(
      reinterpret_cast<int8_t*>(&response->action) + sizeof(AnnounceResponse));

  auto peers_count =
      std::min(static_cast<uint16_t>((max_size - sizeof(AnnounceResponse))
                                     / sizeof(IpV4Port)),
               static_cast<uint16_t>(response->leechers + response->seeders));

  for (size_t i = 0; i < peers_count && ips[i].ip != 0; i++) {
    endpoints.emplace_back(ip::make_address_v4(ips[i].ip),
                           static_cast<uint16_t>(ips[i].port));
  }

  return endpoints;
}

boost::asio::awaitable<void> timeout(time_point& deadline)
{
  steady_timer timer(co_await this_coro::executor);
  auto now = std::chrono::steady_clock::now();

  while (deadline > now) {
    timer.expires_at(deadline);
    co_await timer.async_wait(use_awaitable);
    now = std::chrono::steady_clock::now();
  }
}

boost::asio::awaitable<void> async_receive_from(udp::socket& socket,
                                                std::vector<uint8_t>& buffer,
                                                udp::endpoint& sender,
                                                size_t& bytes_recieved)
{
#pragma warning(push)
#pragma warning(disable : 26811)
  bytes_recieved =
      co_await socket.async_receive_from(boost::asio::buffer(buffer), sender);
#pragma warning(pop)
}

boost::asio::awaitable<size_t> async_receive_from_with_timeout(
    udp::socket& socket,
    std::vector<uint8_t>& buffer,
    udp::endpoint& sender,
    time_point deadline)
{
  size_t bytes_recieved = 0;
  co_await (async_receive_from(socket, buffer, sender, bytes_recieved)
            || timeout(deadline));

  co_return bytes_recieved;
}

boost::asio::awaitable<void> Tracker::fetch_udp_swarm(
    io_context& io, std::vector<udp::endpoint>& out_peer_endpoints, std::chrono::seconds timeout)
{
  auto deadline =
      std::chrono::steady_clock::now() + timeout;

  udp::socket socket(io);

  udp::endpoint remote_endpoint = udp::endpoint(m_address, m_port);

  socket.open(remote_endpoint.protocol());
  std::vector<uint8_t> buffer {};
  buffer.resize(UINT16_MAX);

  {
    ConnectRequest request {};

    co_await socket.async_send_to(
        boost::asio::buffer(&request, sizeof(request)), remote_endpoint);
  }

  udp::endpoint sender {};

  auto handshake_response =
      parse_message<ConnectResponse>(buffer.data(),
                                     co_await async_receive_from_with_timeout(
                                         socket, buffer, sender, deadline));

  if (!handshake_response || sender != remote_endpoint) {
    co_return;
  }

  auto connection_id = (*handshake_response)->connection_id;

  std::fill(buffer.begin(), buffer.end(), 0);

  AnnounceRequest peers_request {*m_context, connection_id};

  co_await socket.async_send_to(
      boost::asio::buffer(&peers_request, sizeof(peers_request)),
      remote_endpoint);

  auto response =
      parse_message<AnnounceResponse>(buffer.data(),
                                      co_await async_receive_from_with_timeout(
                                          socket, buffer, sender, deadline));

  if (!response || sender != remote_endpoint) {
    co_return;
  }

  for (auto ip : parse_ipv4_peer_endpoints(*response, buffer.size())) {
    out_peer_endpoints.emplace_back(ip);
  }

  socket.close();
}
}  // namespace btr
