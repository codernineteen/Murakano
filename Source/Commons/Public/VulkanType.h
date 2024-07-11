#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>
#include <glm/glm.hpp>

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
};

// a struct to wrap a VkImage and its allocation from VMA
struct VkImageAllocated 
{
	VkImage        image;
	VmaAllocation  allocation;
	VmaAllocationInfo allocationInfo;
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
	glm::mat4     modelMatrix;
	glm::vec3     lightPosition;
	unsigned int  objIndex;
	float         lightIntensity;
	int           lightType;
};



// ray push constant
struct VkPushConstantRay
{
	glm::vec4  clearColor;
	glm::vec3  lightPos;
	float      lightIntensity;
	int        lightType;
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