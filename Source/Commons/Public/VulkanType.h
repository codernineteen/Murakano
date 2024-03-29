#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <vector>

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

/**
* Enums
*/

enum VkRtxDescriptorBinding 
{
	TLAS = 0,
	OUT_IMAGE = 1
};