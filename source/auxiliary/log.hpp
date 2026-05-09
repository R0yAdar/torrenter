#pragma once

#include <fmt/core.h>
#include <fmt/color.h>
#include <string_view>

class Log
{
public:
  Log() = delete;

  template<typename... Args>
  static void Info(fmt::format_string<Args...> fmt, Args&&... args)
  {
    fmt::print(fg(fmt::color::cyan), "[INFO] ");
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
  }

  template<typename... Args>
  static void Warn(fmt::format_string<Args...> fmt, Args&&... args)
  {
    fmt::print(fg(fmt::color::pale_violet_red) | fmt::emphasis::bold,
               "[WARN] ");
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
  }

  template<typename... Args>
  static void Error(fmt::format_string<Args...> fmt, Args&&... args)
  {
    fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "[ERROR] ");
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
  }

  template<typename... Args>
  static void Success(fmt::format_string<Args...> fmt, Args&&... args)
  {
    fmt::print(fg(fmt::color::green), "[SUCCESS] ");
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
  }
};