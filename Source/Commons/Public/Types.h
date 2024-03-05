#pragma once

// third-party
#include <vulkan/vulkan.h> // vulkan
#include <fmt/core.h>
#include <DirectXMath.h>
#include <vma/vk_mem_alloc.h>

// std
#include <cstdint>
#include <functional>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>
#include <algorithm>
#include <array>
#include <string>
#include <stack>
#include <queue>

// custom types
#include "VulkanType.h"

// namespace aliases
namespace dx = DirectX;

// integer aliases
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

// function aliases
using VoidLambda = std::function<void(VkCommandBuffer)>;

