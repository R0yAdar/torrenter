#pragma once

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <functional>
#include <random>
#include <string>
#include <string_view>

#include "auxiliary/big_endian.hpp"
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

template<uint8_t Length>
struct PACKED_ATTRIBUTE FixedString
{
  constexpr FixedString(const char (&s)[Length + 1])
  {
    std::copy(s, s + Length, m_data);
  }

  char m_data[Length];
};

struct PACKED_ATTRIBUTE Handshake
{
  Handshake(const PeerContext& context)
  {
    std::copy(context.client_id.get_id().cbegin(),
              context.client_id.get_id().cend(),
              peer_id);

    std::copy(context.infohash.cbegin(), context.infohash.cend(), infohash);
  }

  int8_t plen = 19;
  FixedString<19> pname {"BitTorrent protocol"};
  uint8_t reserved[8];
  char infohash[20];
  char peer_id[20];
};

struct PACKED_ATTRIBUTE Keepalive
{
  int32_big length = 0;
};

template<uint32_t Length, uint8_t Id>
struct PACKED_ATTRIBUTE MessageMetadata
{
  void addLength(uint32_t newLength) { length = length + newLength; }

private:
  uint32_big length = Length;
  uint8_t id = Id;
};

struct PACKED_ATTRIBUTE Have
{
private:
  MessageMetadata<5, 4> metadata;

public:
  uint32_big piece_index;
};

struct PACKED_ATTRIBUTE Request
{
private:
  MessageMetadata<13, 6> metadata;

public:
  uint32_big piece_index;
  uint32_big offset_within_piece;
  uint32_big length;
};

struct PACKED_ATTRIBUTE Cancel
{
private:
  MessageMetadata<13, 8> metadata;

public:
  uint32_big piece_index;
  uint32_big offset_within_piece;
  uint32_big length;
};

struct Port
{
private:
  MessageMetadata<3, 9> metadata;

public:
  uint16_big port;
};

using Choke = MessageMetadata<1, 0>;
using Unchoke = MessageMetadata<1, 1>;
using Interested = MessageMetadata<1, 2>;
using NotInterested = MessageMetadata<1, 3>;

// dynamic length messages

struct PACKED_ATTRIBUTE PieceMetadata
{
public:
  void addLength(int32_t addedLength) { metadata.addLength(addedLength); }

private:
  MessageMetadata<9, 7> metadata;

public:
  uint32_big piece_index;
  uint32_big offset_within_piece;
};

template<typename T>
concept SupportsDynamicLength = requires(T metadata, int32_t someLength) {
  metadata.addLength(someLength);
};

template<SupportsDynamicLength Metadata>
struct DynamicLengthMessage
{
  DynamicLengthMessage(std::vector<std::byte> data)
      : m_payload {std::move(data)}
  {
    metadata.addLength(data.size());
  }

  DynamicLengthMessage(std::vector<std::byte>&& data)
      : m_payload {data}
  {
    metadata.addLength(data.size());
  }

  Metadata metadata;
  const std::vector<std::byte> m_payload;
};

using BitField = DynamicLengthMessage<MessageMetadata<1, 5>>;
using Piece = DynamicLengthMessage<PieceMetadata>;

#pragma pack(pop)

}  // namespace btr