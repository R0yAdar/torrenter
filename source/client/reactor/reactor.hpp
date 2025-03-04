#pragma once

#include <fstream>
#include <memory>
#include <queue>
#include <vector>

#include <boost/asio.hpp>

#include "client/context.hpp"
#include "client/downloader/downloader.hpp"

using namespace std::chrono_literals;

namespace btr
{
class Reactor
{
private:
  std::shared_ptr<InternalContext> m_context;
  std::vector<std::shared_ptr<Downloader>> m_peers;
  std::map<int32_t, std::vector<std::weak_ptr<Downloader>>> m_pieces;
  std::vector<uint8_t> m_file;

public:
  Reactor(std::shared_ptr<InternalContext> context,
          std::vector<std::shared_ptr<Peer>> peers)
      : m_context {context}
      , m_peers {}
      , m_file(context->file_size)
  {
    for (auto& peer : peers) {
      m_peers.push_back(std::make_shared<Downloader>(m_context, peer));
    }
  }

  boost::asio::awaitable<void> assign()
  {
    std::cout << "Piece count: " << m_context->piece_count << std::endl;
    std::cout << "Piece size: " << m_context->piece_size << std::endl;

    std::cout << "File size: " << m_context->file_size << std::endl;

    std::queue<std::weak_ptr<Downloader>> peers {};

    for (auto& peer : m_peers) {
      peers.push(peer);
    }

    for (size_t piece_index = 0; piece_index < m_context->piece_count; piece_index++) {
      auto count = m_pieces[piece_index].size();
      int tries = 0;

      while (count < 3 && tries++ < m_peers.size()) {
        auto peer = peers.front().lock();

        if (peer->get_context().status.remote_bitfield.get(piece_index)) {
          std::cout << peer->get_context().peer_id
                    << " Assigned: " << piece_index << "\n";
          co_await peer->download_piece(piece_index);
          ++count;
          m_pieces[piece_index].push_back(peer);
        }

        peers.pop();
        peers.push(peer);
      }
    }
  }

  boost::asio::awaitable<bool> is_completed()
  {
    std::vector<int32_t> completed_indexes {};

    for (auto& [piece_index, peers] : m_pieces) {
      for (auto& peer : peers) {
        auto locked_peer = peer.lock();
        auto opt_piece = co_await locked_peer->retrieve_piece(piece_index);

        if (opt_piece) {
          FilePiece piece = *opt_piece;
          if (piece.status == PieceStatus::Complete) {
            std::cout << "Finished downloading piece " << piece_index << '\n';

            std::copy(piece.data.cbegin(),
                      piece.data.cend(),
                      m_file.begin() + piece.index * m_context->piece_size);

            completed_indexes.push_back(piece_index);

          } else if (piece.status == PieceStatus::Pending) {
            // std::cout << "Pending piece: " << piece_index << '\n';
          } else if (piece.status == PieceStatus::Active) {
            std::cout << "Active piece: " << piece_index
                      << " downloaded: " << piece.bytes_downloaded << '\n';
          }
        }
      }
    }

    for (auto index : completed_indexes) {
      m_pieces.erase(index);
    }

    co_return m_pieces.size() == 0;
  }

  boost::asio::awaitable<void> download(std::string filepath)
  {
    {
      boost::asio::steady_timer timer(
          co_await boost::asio::this_coro::executor);
      timer.expires_from_now(10s);

      co_await timer.async_wait(boost::asio::use_awaitable);

      co_await assign();
    }
    while (true) {
      boost::asio::steady_timer timer(
          co_await boost::asio::this_coro::executor);
      timer.expires_from_now(10s);

      co_await timer.async_wait(boost::asio::use_awaitable);

      if (co_await is_completed())
        break;
    }
    std::ofstream file(filepath,
                       std::ios::binary);  // Open file in binary mode
    if (!file) {
      std::cerr << "Error opening file for writing: " << filepath << std::endl;
      co_return;
    }
    file.write(reinterpret_cast<const char*>(m_file.data()), m_file.size());
    file.close();
  }
};
}  // namespace btr