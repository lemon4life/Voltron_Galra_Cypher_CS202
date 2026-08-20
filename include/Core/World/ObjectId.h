#pragma once

#include <cstdint>

using ObjectId = std::uint64_t;
using MapObjectHandle = std::uint64_t;

inline constexpr ObjectId INVALID_OBJECT_ID = 0;
inline constexpr MapObjectHandle INVALID_MAP_OBJECT_HANDLE = 0;
