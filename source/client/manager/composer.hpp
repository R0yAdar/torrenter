#pragma once

#include <fstream>
#include <memory>
#include <queue>
#include <vector>

#include <boost/asio.hpp>

#include "client/context.hpp"
#include "client/storage/storage.hpp"
#include "client/tracker/tracker.hpp"
#include "strategy/strategy.hpp"

namespace btr
{
class Composer
{
  std::shared_ptr<InternalContext> m_context;
  std::shared_ptr<IStorage> m_storage_device;
  std::unique_ptr<IStrategy> m_strategy;

public:
  Composer(std::shared_ptr<InternalContext> context,
           std::shared_ptr<IStorage> storage_device);

  boost::asio::awaitable<void> download(std::string filepath,
                                        std::vector<Tracker>& trackers) const;
};
}  // namespace btr