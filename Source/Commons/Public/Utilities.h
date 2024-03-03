#pragma once

// internal
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
	void CreateBuffer(
		VkPhysicalDevice physicalDevice,
		VkDevice logicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory
	);
	/* create an image */
	void CreateImage(
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		uint32 width,
		uint32 height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory
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
