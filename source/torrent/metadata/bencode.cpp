#include <charconv>
#include <cstdlib>
#include <format>
#include <sstream>
#include <system_error>
#include <iostream>

#include "bencode.hpp"

namespace bencode
{

constexpr char INT_PREFIX = 'i';
constexpr char LIST_PREFIX = 'l';
constexpr char DICT_PREFIX = 'd';
constexpr char SUFFIX = 'e';
constexpr char STRING_LENGTH_DELIMITER = ':';
constexpr int MINIMUM_BEVALUE_ENCODED_SIZE = 2;

std::string BEncoder::operator()(BeValue value) const
{
  return boost::apply_visitor(*this, value);
}

std::string BEncoder::operator()(std::int64_t value) const
{
  return INT_PREFIX + std::to_string(value) + SUFFIX;
}

std::string BEncoder::operator()(const std::string& value) const
{
  return std::to_string(value.length()) + STRING_LENGTH_DELIMITER + value;
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

DecodeResult BDecoder::decode_int(std::string_view value) const
{
  auto end_index = value.find_first_of(SUFFIX);
  if (end_index == std::string::npos) {
    throw std::invalid_argument("Corrupted int encoding: no suffix");
  }

  std::int64_t int_value;
  auto result =
      std::from_chars(value.data() + 1, value.data() + end_index, int_value);

  if (result.ec == std::errc::invalid_argument
      || result.ec == std::errc::result_out_of_range)
  {
    throw std::invalid_argument(std::format(
        "Corrupted string encoding: couldn't decode string length, error-code: {}",
        std::to_string(static_cast<int>(result.ec))));
  }

  if (result.ptr != value.data() + end_index) {
    throw std::invalid_argument("Corrupted string encoding: string length");
  }

  return {.result=int_value, .used_chars=++end_index};
}

DecodeResult BDecoder::decode_string(std::string_view value) const
{
  auto index_of_delimiter = value.find_first_of(STRING_LENGTH_DELIMITER);

  if (index_of_delimiter == std::string::npos) {
    throw std::invalid_argument("Corrupted string encoding: no delimiter");
  }

  size_t length;
  auto result =
      std::from_chars(value.data(), value.data() + index_of_delimiter, length);

  if (result.ec == std::errc::invalid_argument
      || result.ec == std::errc::result_out_of_range)
  {
    throw std::invalid_argument(
        "Corrupted string encoding: couldn't decode string length, error-code: "
        + std::to_string(static_cast<int>(result.ec)));
  }

  if (result.ptr != value.data() + index_of_delimiter) {
    throw std::invalid_argument("Corrupted string encoding: string length");
  }

  if (length + (++index_of_delimiter) > value.length()) {
    throw std::invalid_argument(
        "Corrupted string encoding: length doesn't match string");
  }

  return {.result=std::string(value.data() + index_of_delimiter, length),
    .used_chars=index_of_delimiter + length};
}

DecodeResult BDecoder::decode_dict(std::string_view value, int maxDepth) const
{
  Dict values {};
  size_t index = 1;
  while (value[index] != SUFFIX) {
    if (index == value.length()) {
      throw std::invalid_argument("Corrupted list encoding: no suffix");
    }
    auto [key, keyUsedChars] =
        decode_string(value.substr(index, value.length() - index));
    index += keyUsedChars;

    auto [val, valUsedChars] =
        decode(value.substr(index, value.length() - index), maxDepth - 1);

    values[boost::get<std::string>(key)] = val;

    index += valUsedChars;
  }

  return {.result=values, .used_chars=++index};
}

DecodeResult BDecoder::decode_list(std::string_view value, int maxDepth) const
{
  List values {};
  size_t index = 1;
  while (value[index] != SUFFIX) {
    if (index == value.length()) {
      throw std::invalid_argument("Corrupted list encoding: no suffix");
    }
    auto [result, usedChars] =
        decode(value.substr(index, value.length() - index), maxDepth - 1);
    values.push_back(result);
    index += usedChars;
  }

  return {.result=values, .used_chars=++index};
}

DecodeResult BDecoder::decode(std::string_view value, int maxDepth) const
{
  if (maxDepth < 1) {
    throw std::runtime_error("Reached maximum decoding depth");
  }

  if (value.length() < MINIMUM_BEVALUE_ENCODED_SIZE) {
    throw std::invalid_argument("Input length must be greater than minimum");
  }

  switch (value.front()) {
    case INT_PREFIX:
      return decode_int(value);
    case DICT_PREFIX:
      return decode_dict(value, maxDepth);
    case LIST_PREFIX:
      return decode_list(value, maxDepth);
    default:
      if ('0' <= value.front() && value.front() <= '9') {
        return decode_string(value);
      }

      throw std::invalid_argument(
          std::format("Invalid token at front: {}", value[0]));
  }
}

BeValue BDecoder::operator()(std::string_view value, int maxDepth) const
{
  return decode(value, maxDepth).result;
}

}  // namespace bencode