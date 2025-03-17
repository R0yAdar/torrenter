#pragma once

#include <expected>

#define OK(m) \
  ({ \
    auto res = (m); \
    if (!res.has_value()) \
       return unexpected(res.error()); \
    res.value(); \
  })
