#include "Utilities.h"

namespace util
{
    std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open() || file.bad())
            throw std::runtime_error("failed to open file!");

        size_t fileSize = static_cast<size_t>(file.tellg()); // advangtage of ios::ate - we can use read position to determine size of file
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);     // read all bytes at once

        file.close();
        return buffer;
    }

    uint32 FindMemoryType(uint32 typeFilter, VkPhysicalDeviceMemoryProperties deviceMemProperties, VkMemoryPropertyFlags properties)
    {
        for (uint32 i = 0; i < deviceMemProperties.memoryTypeCount; i++)
        {
            // 1. memoryTypeBits is a one-bit bitmask and we can find a match by iterating over each type bit and checking if it is set in the typeFilter
            // 2. If matched index's memory type has all the properties we need, then return the index.
            if ((typeFilter & (1 << i)) && (deviceMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        MK_THROW("failed to find suitable memory type!");
    }

	VkBufferAllocated CreateBuffer(
		const VmaAllocator& allocator,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags memoryAllocationFlags
	)
	{
		// specify buffer creation info
		VkBufferCreateInfo bufferInfo = vkinfo::GetBufferCreateInfo(size, bufferUsage, VK_SHARING_MODE_EXCLUSIVE);

		VmaAllocationCreateInfo bufferAllocInfo{};
		bufferAllocInfo.usage = memoryUsage;
		/**
		* CREATE_MAPPED_BIT by default 
		*  - flag for automatic mapping pointer with 'pMappedData' member of VmaAllocationInfo struct (as long as buffer is accessible from CPU )
		*/
		bufferAllocInfo.flags = memoryAllocationFlags;

		// create buffer
		VkBufferAllocated newBuffer{};
		MK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &bufferAllocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.allocationInfo));

		return newBuffer;
	}

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
	)
	{
		// specify image creation info
		VkImageCreateInfo imageInfo = vkinfo::GetImageCreateInfo(width, height, format, tiling, usage);

		VmaAllocationCreateInfo imageAllocInfo{};
		imageAllocInfo.usage = memoryUsage;
		imageAllocInfo.flags = memoryAllocationFlags;

		MK_CHECK(vmaCreateImage(allocator, &imageInfo, &imageAllocInfo, &newImage.image, &newImage.allocation, nullptr));
	}

	void CreateImageView(
		VkDevice logicalDevice,
		VkImage image,
		VkImageView& imageView,
		VkFormat imageFormat,
		VkImageAspectFlags aspectFlags,
		uint32 mipLevels
	)
	{
		VkImageViewCreateInfo imageViewCreateInfo = vkinfo::GetImageViewCreateInfo(image, imageFormat, aspectFlags, mipLevels);
		MK_CHECK(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView));
	}

	// Find supported device format
	VkFormat FindSupportedTilingFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			/**
			* VkFormatProperties specification
			* 1. linearTilingFeatures : use cases that are supported with linear tiling
			* 2. optimalTilingFeatures : use cases that are supported with optimal tiling
			* 3. bufferFeatures : use cases that are supported for buffers
			*/
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)         // when linear tiling is required
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)  // when optimal tiling is required
				return format;
		}

		MK_THROW("failed to find supported tiling format!");
	}

	// Find depth-specific format
	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
	{
		std::vector<VkFormat> depthFormatCandidates{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		return util::FindSupportedTilingFormat(
			physicalDevice,
			depthFormatCandidates,
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
}