#pragma once

#define GLM_ENABLE_EXPERIMENTAL

// third-party
#include <vulkan/vulkan.h>               // vulkan header
#include <fmt/core.h>                    // fmt lib for logging
#include <DirectXMath.h>                 // directx math
#include <DirectXPackedVector.h>         // directx packed vector
#include <vma/vk_mem_alloc.h>            // vulkan memory allocator from GPUopen
#include <glm/glm.hpp>                   // glm math
#include <glm/gtc/matrix_transform.hpp>  // glm matrix transform
#include <glm/gtx/hash.hpp>              // glm hash

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
using namespace DirectX;
using namespace DirectX::PackedVector;

// integer aliases
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

// function aliases
using VoidLambda = std::function<void(VkCommandBuffer)>;

