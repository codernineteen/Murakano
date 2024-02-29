#pragma once

#include <cstdint>
#include <functional>

// integer aliases
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

// function aliases
using VoidLambda = std::function<void(VkCommandBuffer)>;
