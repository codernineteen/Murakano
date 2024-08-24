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

// buffer usage flags
const VkBufferUsageFlags VK_DEVICE_ADDRESS_USAGE_FLAG = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
const VkBufferUsageFlags VK_RAYTRACING_USAGE_FLAG = VK_DEVICE_ADDRESS_USAGE_FLAG | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
const VkDescriptorPoolSize VK_GLOBAL_DESCRIPTOR_POOL_SIZE = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };

// shader stage flags
const VkShaderStageFlags VK_SHADER_VS_FS_FLAG = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

/**
* Enums
*/

enum ELightType
{
	POINT_LIGHT = 0,
	DIRECTIONAL_LIGHT = 1,
	SPOT_LIGHT = 2
};

enum EShaderBinding
{
	UNIFORM_BUFFER = 0,
	TEXTURE_2D_ARRAY = 1,
	SAMPLER = 2,
	COMBINED_IMAGE_SAMPLER = 3,
};

enum ERtDescriptorBinding
{
	TLAS = 2,
	OUT_IMAGE = 3
};

enum ERtShaderStages
{
	RAYGEN,
	MISS,
	CLOSEST_HIT,
	STAGE_COUNT
};



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
	VkImage           image = VK_NULL_HANDLE;
	VkFormat          format;
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
	ELightType lightType;
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
