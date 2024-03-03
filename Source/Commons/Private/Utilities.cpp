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

	void CreateBuffer(
		VkPhysicalDevice physicalDevice,
		VkDevice logicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory
	)
	{
		// specify buffer creation info
		VkBufferCreateInfo bufferInfo = vkinfo::GetBufferCreateInfo(size, bufferUsage, VK_SHARING_MODE_EXCLUSIVE);

		MK_CHECK(vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer));

		// query memory requirements for the buffer
		/**
		* VKMemoryRequirements specification
		* 1. size : the size of the required amount of memory in bytes
		* 2. alignment : the offset in bytes where the buffer begins in the allocated region of memory
		* 3. memoryTypeBits : a bitmask and contains one bit set for every supported memory type for the resource. 'Bit i' is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties structure for the physical device is supported for the resource.
		*/
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);
		/**
		* VKPhysicalDeviceMemoryProperties specification
		* 1. memoryTypes : different types of memory like device local, host visible, coherent, and cached
		* 2. memoryHeaps : distinct memory resources like dedicated VRAM and swap space in RAM for example (this can affect performance)
		*/
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// find a memory type that is suitable for the buffer and meets the requirements
		uint32 memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memProperties, properties);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;

		// allocate memory for the buffer
		MK_CHECK(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory));
		// bind allocated memory with the buffer by calling 'vkBindBufferMemory'
		MK_CHECK(vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0));
	}

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
	)
	{
		// specify image creation info
		VkImageCreateInfo imageInfo = vkinfo::GetImageCreateInfo(width, height, format, tiling, usage);
		MK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &image));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		// query device memory properties
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memProperties, properties);

		MK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
		MK_CHECK(vkBindImageMemory(device, image, imageMemory, 0));
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