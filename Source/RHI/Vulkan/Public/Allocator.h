#pragma once

#include <vma/vk_mem_alloc.h>
#include <fmt/format.h>

#include "VulkanType.h"
#include "Info.h"
#include "Macros.h"

class Allocator
{
public:
	Allocator();
	~Allocator();

public:
	/* initializer */
	void InitVMAAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

public:
	/* allocation APIs */
	VkBufferAllocated CreateBuffer(
		VkDeviceSize             size, 
		VkBufferUsageFlags       bufferUsage, 
		VmaMemoryUsage           memoryUsage, 
		VmaAllocationCreateFlags memoryAllocationFlags,
		std::string              allocationName = "UNDEFINED"
	) const;
	VkImageAllocated CreateImage(
		uint32                   width,
		uint32                   height,
		VkFormat                 format,
		VkImageTiling            tiling,
		VkImageUsageFlags        usage,
		VmaMemoryUsage           memoryUsage,
		VmaAllocationCreateFlags memoryAllocationFlags,
		VkImageLayout            layout = VK_IMAGE_LAYOUT_UNDEFINED,
		std::string              allocationName = "UNDEFINED"
	) const;

public:
	/* destruction APIs */
	void DestroyBuffer(VkBufferAllocated& bufferAllocated) const;
	void DestroyImage(VkImageAllocated& imageAllocated) const;

private:
	VmaAllocator _vmaAllocator = VMA_NULL;
};