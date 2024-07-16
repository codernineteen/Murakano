#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#define HLSL

using namespace DirectX;
using namespace DirectX::PackedVector;
/**
* Constants
*/

const VkBufferUsageFlags vkDeviceAddressFlag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
const VkBufferUsageFlags vkRayTracingFlags = vkDeviceAddressFlag | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

/**
* Structs
*/

// a struct to wrap a Vkbuffer and its allocation from VMA
struct VkBufferAllocated 
{
	VkBuffer           buffer;
	VmaAllocation      allocation;
	VmaAllocationInfo  allocationInfo;
	std::string        name;
};

// a struct to wrap a VkImage and its allocation from VMA
struct VkImageAllocated 
{
	VkImage           image;
	VmaAllocation     allocation;
	VmaAllocationInfo allocationInfo;
	std::string       name;
};

// Acceleration structure for the raytracer
struct VkAccelKHR
{
	VkAccelerationStructureKHR  handle = VK_NULL_HANDLE;
	VkBufferAllocated           buffer;
};

// only used as reference from the TLAS
struct VkBLAS
{
	/* The shape and type of the acceleration structure */
	std::vector<VkAccelerationStructureGeometryKHR>        geometry;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR>  buildRangeInfo;
	VkBuildAccelerationStructureFlagsKHR                   flags{ 0 };
};

// a storage image abstraction
struct VkStorageImage
{
	VkImageAllocated imageAllocated;
	VkImageView      imageView;
	VkFormat         format;
};

enum LightType
{
	POINT_LIGHT = 0,
	DIRECTIONAL_LIGHT = 1,
	SPOT_LIGHT = 2
};

// rasterization push constant
struct VkPushConstantRaster
{
#ifdef HLSL
	XMMATRIX  modelMat;
	XMVECTOR  lightPosition;
#else
	glm::mat4 modelMat;
	glm::vec3 lightPosition;
#endif
	float     lightIntensity;
	LightType lightType;
};

// ray push constant
struct VkPushConstantRay
{
#ifdef HLSL
	XMVECTOR  lightPos;
#else
	glm::vec3 lightPos;
#endif
	glm::vec4 clearColor;
	float     lightIntensity;
	int       lightType;
};

/**
* Enums
*/

enum ShaderBinding
{
	UNIFORM_BUFFER = 0,
	TEXTURE_SAMPLER = 1,
};

enum VkRtxDescriptorBinding 
{
	TLAS = 2,
	OUT_IMAGE = 3
};

enum VkRtxShaderStages
{
	RAYGEN,
	MISS,
	CLOSEST_HIT,
	STAGE_COUNT
};