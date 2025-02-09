#pragma once

#include <memory>
#include <vector>

#include "client/context.hpp"

namespace btr
{
enum class PieceStatus
{
  Pending,
  Assigned,
  Completed,
};

struct Piece
{
  PieceStatus status;
};

struct ThrottlePolicy
{
};

class Reactor
{
private:
  std::shared_ptr<InternalContext> m_context;
  std::vector<std::weak_ptr<ExternalPeerContext>> m_available_peers;
  std::vector<Piece> m_pieces;
  ThrottlePolicy m_policy;

public:
  Reactor(std::shared_ptr<InternalContext> context, ThrottlePolicy policy) {}

  void assign();

  void on_downloaded_piece(const ExternalPeerContext& peer, ...);
};
}  // namespace btr