#pragma once

#include <bit>
#include <chrono>
#include "auxiliary/big_endian.hpp"
#include "auxiliary/random.hpp"

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

/*
0       4       8               16              24              32 (bits)
+-------+-------+---------------+---------------+---------------+
| type  | ver   | extension     | connection_id                 |
+-------+-------+---------------+---------------+---------------+
| timestamp_microseconds                                        |
+---------------+---------------+---------------+---------------+
| timestamp_difference_microseconds                             |
+---------------+---------------+---------------+---------------+
| wnd_size                                                      |
+---------------+---------------+---------------+---------------+
| seq_nr                        | ack_nr                        |
+---------------+---------------+---------------+---------------+
*/

enum ProtocolVersion : uint8_t
{
  Version1 = 1
};

enum PacketType : uint8_t
{
  ST_DATA = 0,  // Regular data packet, always has a payload
  ST_FIN = 1,  // Finalize the connection, is the last packet. seq_nr of this
               // packet will be the last packet expected (though earlier
               // packets may still be in flight)
  ST_STATE = 2,  // State packet, used to transmit an acknowledgement of a
                 // packet, does not increase the seq_nr
  ST_RESET = 3,  // Force closes the connection
  ST_SYN = 4,  // Connect/Sync packet, seq_nr is set to 1, connection_id is
               // randomised, all subsequent packets sent on this connection are
               // sent with the connection_id + 1
};

class PACKED_ATTRIBUTE TypeVersion
{
public:
  TypeVersion(PacketType type, ProtocolVersion version)
  {
    m_typeVersion = static_cast<byte>((static_cast<byte>(type) << 4)
                                      | static_cast<byte>(version));
  }

  operator byte() { return m_typeVersion; }

private:
  byte m_typeVersion;
};

class PACKED_ATTRIBUTE UniqueConnectionID
{
public:
  UniqueConnectionID()
      : m_connectionID {generate_random_in_range<int16_t, INT16_MIN, INT16_MAX>()}
  {
  }

  operator int16_big() { return m_connectionID; }

private:
  int16_big m_connectionID;
};

class PACKED_ATTRIBUTE Timestamp
{
public:
  Timestamp()
      : m_timestamp {0}
  {
  }
  /*
  operator std::chrono::time_point<std::chrono::system_clock>()
  {
    return std::chrono::time_point<std::chrono::system_clock>();
  }
  */
private:
  int32_big m_timestamp;
};

struct PACKED_ATTRIBUTE UtpPacket
{
public:
  TypeVersion typeVersion;
  byte extension;
  UniqueConnectionID connectionID;
  Timestamp timestampMicroseconds;
  Timestamp timestampDifferenceMicroseconds;
  int32_big wndSize;
  int16_big seqNr;
  int16_big ackNr;
};

#pragma pack(pop)