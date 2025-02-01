#pragma once
#include <array>
#include <random>
#include <string_view>

using std::byte;
using random_bytes_engine = std::
    independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned int>;
/*
 * Impersonate a qBittorrent client (-qBXYZ0-<12 random bytes>)
 */
class PeerId
{
public:
  PeerId()
      : m_peer_id {}
  {
    m_peer_id[0] = '-';
    m_peer_id[1] = 'q';
    m_peer_id[2] = 'B';
    m_peer_id[3] = '1';
    m_peer_id[4] = '0';
    m_peer_id[5] = '1';
    m_peer_id[6] = '0';
    m_peer_id[7] = '-';

    random_bytes_engine rbe;
    std::generate(m_peer_id.begin() + 8, m_peer_id.end(), rbe);
  }

  const std::array<uint8_t, 20>& get_id() const { return m_peer_id; }

private:
  std::array<uint8_t, 20> m_peer_id;
};