#include <charconv>
#include <cstdlib>
#include <format>
#include <sstream>
#include <system_error>

#include "bencode.hpp"

namespace bencode
{
const char INT_PREFIX = 'i';
const char LIST_PREFIX = 'l';
const char DICT_PREFIX = 'd';
const char SUFFIX = 'e';
const char STRING_LENGTH_DELIMETER = ':';
const int MINIMUM_BEVALUE_ENCODED_SIZE = 2;

std::string BEncoder::operator()(BeValue value) const
{
  return boost::apply_visitor(*this, value);
}

std::string BEncoder::operator()(std::int64_t value) const
{
  return INT_PREFIX + std::to_string(value) + SUFFIX;
}

std::string BEncoder::operator()(std::string value) const
{
  return std::to_string(value.length()) + STRING_LENGTH_DELIMETER + value;
}

std::string BEncoder::operator()(const List& values) const
{
  std::stringstream stream;
  stream << LIST_PREFIX;

  for (auto& value : values) {
    stream << boost::apply_visitor(*this, value);
  }

  stream << SUFFIX;

  return stream.str();
}

std::string BEncoder::operator()(const Dict& values) const
{
  std::stringstream stream;
  stream << DICT_PREFIX;

  for (auto& value : values) {
    stream << (*this)(value.first) << boost::apply_visitor(*this, value.second);
  }

  stream << SUFFIX;

  return stream.str();
}

DecodeResult BDecoder::decodeInt(std::string_view value) const
{
  auto endIndex = value.find_first_of(SUFFIX);
  if (endIndex == std::string::npos) {
    throw std::invalid_argument("Corrupted int encoding: no suffix");
  }

  std::int64_t intValue;
  auto result =
      std::from_chars(value.data() + 1, value.data() + endIndex, intValue);

  if (result.ec == std::errc::invalid_argument
      || result.ec == std::errc::result_out_of_range)
  {
    throw std::invalid_argument(std::format(
        "Corrupted string encoding: couldn't decode string length, errc: {}",
        static_cast<int>(result.ec)));
  }

  if (result.ptr != value.data() + endIndex) {
    throw std::invalid_argument("Corrupted string encoding: string length");
  }

  return {intValue, ++endIndex};
}

DecodeResult BDecoder::decodeString(std::string_view value) const
{
  auto delimitIndex = value.find_first_of(STRING_LENGTH_DELIMETER);

  if (delimitIndex == std::string::npos) {
    throw std::invalid_argument("Corrupted string encoding: no delimiter");
  }

  size_t length;
  auto result =
      std::from_chars(value.data(), value.data() + delimitIndex, length);

  if (result.ec == std::errc::invalid_argument
      || result.ec == std::errc::result_out_of_range)
  {
    throw std::invalid_argument(
        "Corrupted string encoding: couldn't decode string length, errc: "
        + (char)result.ec);
  }

  if (result.ptr != value.data() + delimitIndex) {
    throw std::invalid_argument("Corrupted string encoding: string length");
  }

  return {std::string(value.data() + (++delimitIndex), length),
          delimitIndex + length};
}

DecodeResult BDecoder::decodeDict(std::string_view value) const
{
  Dict values {};
  size_t index = 1;
  while (value[index] != SUFFIX) {
    if (index == value.length()) {
      throw std::invalid_argument("Corrupted list encoding: no suffix");
    }
    auto [key, keyUsedChars] =
        decodeString(value.substr(index, value.length() - index));
    index += keyUsedChars;

    auto [val, valUsedChars] =
        decode(value.substr(index, value.length() - index));

    values[boost::get<std::string>(key)] = val;

    index += valUsedChars;
  }

  return {values, ++index};
}

DecodeResult BDecoder::decodeList(std::string_view value) const
{
  List values {};
  size_t index = 1;
  while (value[index] != SUFFIX) {
    if (index == value.length()) {
      throw std::invalid_argument("Corrupted list encoding: no suffix");
    }
    auto [result, usedChars] =
        decode(value.substr(index, value.length() - index));
    values.push_back(result);
    index += usedChars;
  }

  return {values, ++index};
}

DecodeResult BDecoder::decode(std::string_view value) const
{
  if (value.length() < MINIMUM_BEVALUE_ENCODED_SIZE) {
    throw std::invalid_argument("Input length must be greater than minium");
  }

  switch (value.front()) {
    case INT_PREFIX:
      return decodeInt(value);
    case DICT_PREFIX:
      return decodeDict(value);
    case LIST_PREFIX:
      return decodeList(value);
    default:
      if ('0' <= value.front() && value.front() <= '9') {
        return decodeString(value);
      }
      throw std::invalid_argument(
          std::format("Invalid token at front: {}", value[0]));
  }
}

BeValue BDecoder::operator()(std::string_view value) const
{
  return decode(value).result;
}

}  // namespace bencode