#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

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

// a struct to wrap a VkDescriptorType and its ratio in pool
struct VkDescriptorPoolSizeRatio
{
	VkDescriptorType  type;
	float             ratio;
};