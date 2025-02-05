#include <filesystem>
#include <iostream>
#include <memory>
#include <print>
#include <regex>

#include "client/torrenter.hpp"

#include <boost/asio.hpp>

#include "client/peer.hpp"
#include "client/tracker/tracker.hpp"

namespace btr
{
Torrenter::Torrenter(TorrentFile torrent)
    : m_torrent {torrent}
    , m_peer_id {}
{
}
void Torrenter::download_file(std::filesystem::path at)
{
  std::regex rgx(R"(udp://([a-zA-Z0-9.-]+):([0-9]+)/announce)");

  std::vector<boost::asio::ip::udp::endpoint> endpoints {};
  std::smatch match;

  io_context io_1 {};

  for (auto tracker : m_torrent.trackers) {
    std::println("{}", tracker);

    if (std::regex_search(tracker, match, rgx)) {
      std::string address = match[1];
      int16_t port = static_cast<int16_t>(std::stoi(match[2]));

      boost::asio::co_spawn(
          io_1,
          [&io_1, &endpoints, address, port]() -> boost::asio::awaitable<void>
          {
            boost::asio::ip::udp::resolver resolver(io_1);
            boost::asio::ip::udp::resolver::query query(
                boost::asio::ip::udp::v4(), address, std::to_string(port));
            boost::asio::ip::udp::endpoint endpoint =
                *(co_await resolver.async_resolve(query));
            endpoints.push_back(endpoint);
            std::cout << endpoint << std::endl;
          },
          boost::asio::detached);
    }
  }

  io_1.run();

  for (auto& endpoint : endpoints) {
    io_context io {};

    auto swarm =
        btr::udp_fetch_swarm(*this, endpoint.address(), endpoint.port());

    auto context = std::make_shared<PeerContext>();

    context->client_id = m_peer_id;
    context->infohash = m_torrent.info_hash;

    std::vector<Peer> peers;

    if (swarm) {
      for (const auto& endpoint : *swarm) {
        Peer peer {context, endpoint.address(), endpoint.port()};
        peers.push_back(peer);
      }

      for (auto& peer : peers) {
        boost::asio::co_spawn(
            io, peer.connect_async(io), boost::asio::detached);
      }

      io.run();
    }
  }
}
}  // namespace btr