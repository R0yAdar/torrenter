#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "auxiliary/big_endian.hpp"
#include "auxiliary/random.hpp"
#include "client/context.hpp"

namespace btr
{
#ifndef PACKED_ATTRIBUTE
#  if defined BROKEN_GCC_STRUCTURE_PACKING && defined __GNUC__
// Used for gcc tool chains accepting but not supporting pragma pack
// See http://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html
#    define PACKED_ATTRIBUTE __attribute__((__packed__))
#  else
#    define PACKED_ATTRIBUTE
#  endif  // defined BROKEN_GCC_STRUCTURE_PACKING && defined __GNUC__
#endif  // ndef PACKED_ATTRIBUTE

#pragma pack(push, 1)

/// Announce protocol. messages could grow more.

enum class Actions : int32_t
{
  Connect = 0,
  Announce = 1,
  Scrape = 2,
  Error = 3,  // Only sent by tracker to client
};

constexpr uint64_t ANNOUNCER_MAGIC = 0x41727101980;

struct PACKED_ATTRIBUTE ConnectRequest
{
  uint64_big protocol_id = ANNOUNCER_MAGIC;
  uint32_big action = static_cast<uint32_t>(Actions::Connect);
  uint32_big transaction_id =
      generate_random_in_range<uint32_t, 0, UINT32_MAX>();
};

struct PACKED_ATTRIBUTE ConnectResponse
{
  uint32_big action;
  uint32_big transaction_id;
  uint64_big connection_id;
};

struct PACKED_ATTRIBUTE AnnounceRequest
{
  AnnounceRequest(const InternalContext& context, uint64_big p_connection_id)
  {
    connection_id = p_connection_id;
    action = static_cast<uint32_t>(Actions::Announce);
    transaction_id = generate_random_in_range<uint32_t, 0, UINT32_MAX>();

    auto id = context.id();

    std::copy(id.as_raw().cbegin(),
              id.as_raw().cend(),
              peer_id);

    std::copy(context.info_hash.cbegin(), context.info_hash.cend(), info_hash);

    downloaded = 0;
    left = context.file_size;
    key = generate_random_in_range<uint32_t, 0, UINT32_MAX>();
    port = 2929;
  }

  uint64_big connection_id;
  uint32_big action;
  uint32_big transaction_id;

  uint8_t info_hash[20];
  uint8_t peer_id[20];
  uint64_big downloaded;
  uint64_big left;
  uint64_big uploaded;
  uint32_big event = 0;  // 0: none; 1: completed; 2: started; 3: stopped
  uint32_big ip_address = 0;  // default, your ip, 0 = this ip
  uint32_big key;  // random unique key
  int32_big num_want = -1;  // num of peers in reply (-1 = default)
  uint16_big port;  // your listening port
};

struct PACKED_ATTRIBUTE IpV4Port
{
  uint32_big ip;
  uint16_big port;
};

struct PACKED_ATTRIBUTE AnnounceResponse
{
  uint32_big action;
  uint32_big transaction_id;
  uint32_big interval;
  uint32_big leechers;
  uint32_big seeders;
  // IpV4Port[N]... (N = leechers + seeders)
};

struct PACKED_ATTRIBUTE ErrorResponse
{
  uint32_big action = static_cast<uint32_t>(Actions::Error);
  uint32_big transaction_id;
  std::string message;
};

#pragma pack(pop)
}  // namespace btr