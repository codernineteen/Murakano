#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

// a struct to wrap a Vkbuffer and its allocation from VMA
struct VkBufferAllocated 
{
	VkBuffer          buffer;
	VmaAllocation     allocation;
	VmaAllocationInfo allocationInfo;
};