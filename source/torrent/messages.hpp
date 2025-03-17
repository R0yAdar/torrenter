#pragma once

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <tuple>
#include <variant>

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

constexpr uint8_t ID_KEEPALIVE = 255;
constexpr uint8_t ID_CHOKE = 0;
constexpr uint8_t ID_UNCHOKE = 1;
constexpr uint8_t ID_INTERESTED = 2;
constexpr uint8_t ID_NOT_INTERESTED = 3;
constexpr uint8_t ID_HAVE = 4;
constexpr uint8_t ID_BITFIELD = 5;
constexpr uint8_t ID_REQUEST = 6;
constexpr uint8_t ID_PIECE = 7;
constexpr uint8_t ID_CANCEL = 8;
constexpr uint8_t ID_PORT = 9;

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
  Handshake(const InternalContext& context)
  {
    auto id = context.id();
    std::copy(id.as_raw().cbegin(), id.as_raw().cend(), peer_id);

    std::copy(context.info_hash.cbegin(), context.info_hash.cend(), infohash);
  }

  Handshake() = default;

  int8_t plen = 19;
  FixedString<19> pname {"BitTorrent protocol"};
  uint8_t reserved[8] {0};
  char infohash[20];
  char peer_id[20];
};

struct PACKED_ATTRIBUTE Keepalive
{
  int32_big length = 0;
};

template<uint32_t Length = 0, uint8_t Id = -1>
struct PACKED_ATTRIBUTE MessageMetadata
{
  void add_length(uint32_t some_length) { length = length + some_length; }

  uint32_big length = Length;
  uint8_t id = Id;
};

struct PACKED_ATTRIBUTE Have
{
private:
  MessageMetadata<5, ID_HAVE> metadata;

public:
  uint32_big piece_index;
};

struct PACKED_ATTRIBUTE Request
{
  Request() = default;
  Request(uint32_t piece_index, uint32_t offset_within_piece, uint32_t length)
      : piece_index {piece_index}
      , offset_within_piece {offset_within_piece}
      , length {length}
  {
  }

private:
  MessageMetadata<13, ID_REQUEST> metadata;

public:
  uint32_big piece_index;
  uint32_big offset_within_piece;
  uint32_big length;
};

struct PACKED_ATTRIBUTE Cancel
{
private:
  MessageMetadata<13, ID_CANCEL> metadata;

public:
  uint32_big piece_index;
  uint32_big offset_within_piece;
  uint32_big length;
};

struct Port
{
private:
  MessageMetadata<3, ID_PORT> metadata;

public:
  uint16_big port;
};

using Choke = MessageMetadata<1, ID_CHOKE>;
using Unchoke = MessageMetadata<1, ID_UNCHOKE>;
using Interested = MessageMetadata<1, ID_INTERESTED>;
using NotInterested = MessageMetadata<1, ID_NOT_INTERESTED>;

// dynamic length messages

struct PACKED_ATTRIBUTE PieceMetadata
{
  void add_length(uint32_t some_length) { metadata.add_length(some_length); }

private:
  MessageMetadata<9, ID_PIECE> metadata;

public:
  uint32_big piece_index;
  uint32_big offset_within_piece;
};

template<typename T>
concept SupportsDynamicLength = requires(T metadata, uint32_t some_length) {
  metadata.add_length(some_length);
};

template<SupportsDynamicLength Metadata>
struct DynamicLengthMessage
{
  DynamicLengthMessage(std::vector<uint8_t> data, Metadata metadata = {})
      : m_metadata {metadata}
      , m_payload {std::move(data)}
  {
    m_metadata.add_length(m_payload.size());
  }

  std::vector<uint8_t> get_payload() const { return m_payload; }

  Metadata get_metadata() const { return m_metadata; }

private:
  Metadata m_metadata;
  std::vector<uint8_t> m_payload;
};

using BitField = DynamicLengthMessage<MessageMetadata<1, ID_BITFIELD>>;
using Piece = DynamicLengthMessage<PieceMetadata>;

using TorrentMessage = std::variant<Keepalive,
                                    Choke,
                                    Unchoke,
                                    Interested,
                                    NotInterested,
                                    Have,
                                    BitField,
                                    Request,
                                    Piece,
                                    Cancel,
                                    Port>;

#pragma pack(pop)

}  // namespace btr