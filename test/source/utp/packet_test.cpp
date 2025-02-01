#include "utp/packet.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("UtpPacket packed correctly", "[library]")
{
  REQUIRE(sizeof(UtpPacket) == 20);
}

TEST_CASE("Big Endian is converted correctly", "[library]")
{
  byte expected[] {0x12, 0x34, 0x56, 0x78};
  int32_big val = 0x12345678;

  struct Aux
  {
    int32_big value;
  };

  Aux check {val};
  auto ptr = static_cast<byte*>(static_cast<void*>(&check));

  REQUIRE(val == 0x12345678);
  REQUIRE(sizeof(Aux) >= sizeof(expected));

  for (size_t i = 0; i < sizeof(expected); i++) {
    REQUIRE(*(ptr++) == expected[i]);
  }
}

TEST_CASE("TypeVersion", "[library]")
{
  REQUIRE((byte)TypeVersion {PacketType::ST_DATA, ProtocolVersion::Version1}
          == 0x01);
  REQUIRE((byte)TypeVersion {PacketType::ST_FIN, ProtocolVersion::Version1}
          == 0x11);
  REQUIRE((byte)TypeVersion {PacketType::ST_STATE, ProtocolVersion::Version1}
          == 0x21);
  REQUIRE((byte)TypeVersion {PacketType::ST_RESET, ProtocolVersion::Version1}
          == 0x31);
  REQUIRE((byte)TypeVersion {PacketType::ST_SYN, ProtocolVersion::Version1}
          == 0x41);
}