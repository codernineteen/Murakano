#pragma once

#include "Types.h"
#include "Conversion.h"
#include "Macros.h"
#include "Info.h"

// helper function
namespace util 
{
	/* read local file */
	std::vector<char> ReadFile(const std::string& filename);
	
	/* a utility to find a suitable memory type*/
	uint32 FindMemoryType(uint32 typeFilter, VkPhysicalDeviceMemoryProperties deviceMemProperties, VkMemoryPropertyFlags properties);

	/* create a buffer */
	VkBufferAllocated CreateBuffer(
		const VmaAllocator& allocator,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags memoryAllocationFlags
	);

	/* create an image */
	void CreateImage(
		const VmaAllocator& allocator,
		VkImageAllocated& newImage,
		uint32 width,
		uint32 height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags memoryAllocationFlags
	);

	/* create single image view related to the given image parameter */
	void CreateImageView(
		VkDevice logicalDevice,
		VkImage image,
		VkImageView& imageView,
		VkFormat imageFormat,
		VkImageAspectFlags aspectFlags,
		uint32 mipLevels
	);

	/* find supported device format */
	VkFormat FindSupportedTilingFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	/* find depth - specific format */
	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);
}
