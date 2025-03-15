#include <filesystem>
#include <iostream>
#include <memory>
#include <print>
#include <regex>
#include <set>

#include "client/torrenter.hpp"

#include <boost/asio.hpp>

#include "client/peer.hpp"
#include "client/tracker/tracker.hpp"
#include "client/reactor/reactor.hpp"

namespace btr
{
Torrenter::Torrenter(TorrentFile torrent)
    : m_torrent {torrent}
{
}

// Download file is async and not multi-threaded
// Because multi-threading is obsolete for an io bound task like this
// And Boost-Asio provides a wonderful interface for high-performance
// async services.
void Torrenter::download_file(std::filesystem::path at)
{
  std::regex rgx(R"(udp://([a-zA-Z0-9.-]+):([0-9]+)/announce)");

  std::vector<boost::asio::ip::udp::endpoint> endpoints {};
  std::smatch match;

  io_context resolve_io {};

  for (auto tracker : m_torrent.trackers) {
    if (std::regex_search(tracker, match, rgx)) {
      std::string address = match[1];
      int16_t port = static_cast<int16_t>(std::stoi(match[2]));

      boost::asio::co_spawn(
          resolve_io,
          [&resolve_io, &endpoints, address, port]()
              -> boost::asio::awaitable<void>
          {
            boost::asio::ip::udp::resolver resolver(resolve_io);
            boost::asio::ip::udp::resolver::query query(
                boost::asio::ip::udp::v4(), address, std::to_string(port));
            boost::asio::ip::udp::endpoint endpoint =
                *(co_await resolver.async_resolve(query));
            endpoints.push_back(endpoint);
          },
          boost::asio::detached);
    }
  }

  resolve_io.run();


  std::vector<PeerContactInfo> peers;

  io_context connect_io {};

  auto context = std::make_shared<InternalContext>();

  context->info_hash = std::vector(m_torrent.info_hash.cbegin(),
                                            m_torrent.info_hash.cend());
  context->file_size = m_torrent.file_length;
  context->piece_size = m_torrent.piece_length;
  context->piece_count = m_torrent.piece_hashes.size();

  std::cout << "FileSize: " << context->file_size << std::endl;

  std::vector<Tracker> trackers;

  for (auto& endpoint : endpoints) {
    trackers.emplace_back(context, endpoint.address(), endpoint.port());
  }

  Reactor reactor {context};

  boost::asio::co_spawn(connect_io,
                        reactor.download(at.generic_string(), trackers),
                        boost::asio::detached);

  connect_io.run();

  // bitfield and assignment

  // task managing, downloading pieces (& validating)

  // THE END (game)

  // Final end
}
}  // namespace btr