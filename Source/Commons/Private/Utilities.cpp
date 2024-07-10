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
		VmaAllocationCreateFlags memoryAllocationFlags,
		VkImageLayout layout
	)
	{
		// specify image creation info
		VkImageCreateInfo imageInfo = vkinfo::GetImageCreateInfo(width, height, format, tiling, usage, layout);

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

	VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32*>(code.data()); // guaranteed to be aligned by default allocator  of std::vector 

		VkShaderModule shaderModule;
		MK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

		return shaderModule;
	}

	void DestroyImageResource(const VmaAllocator& allocator, const VkDevice& device, VkImageAllocated& imageAllocated, VkImageView& imageView)
	{
		vmaDestroyImage(allocator, imageAllocated.image, imageAllocated.allocation);
		vkDestroyImageView(device, imageView, nullptr);
	}

	// Find supported device format
	VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
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

	// Find depth stencil format
	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice)
	{
		return FindSupportedFormat(
			physicalDevice,
			{
				VK_FORMAT_X8_D24_UNORM_PACK32,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D16_UNORM,
				VK_FORMAT_D16_UNORM_S8_UINT,
			},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
	}

	VkFormat FindDepthStencilFormat(VkPhysicalDevice physicalDevice)
	{
		return FindSupportedFormat(
			physicalDevice,
			{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
	
	// copy buffer to image
	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkBufferImageCopy region{};
		// which part of buffer to copy
		region.bufferOffset = 0;
		region.bufferRowLength = 0;   // specify how the pixels are laid out in memory(1)
		region.bufferImageHeight = 0; // specify how the pixels are laid out in memory(2)

		// to which part of image
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1 // depth 1 because texture image is 2D
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // specify the layout of the image assuming image has already been transitioned to the layout
			1,
			&region
		);
	}

	// copy image to image
	void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) 
	{
		// TODO : remove hard-coded subresource range
		VkImageCopy copy_region{};
		copy_region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copy_region.srcOffset = { 0, 0, 0 };
		copy_region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copy_region.dstOffset = { 0, 0, 0 };
		copy_region.extent = { width, height, 1 }; // depth : 1

		vkCmdCopyImage(
			commandBuffer,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &copy_region
		);
	}

	VkAccessFlags GetAccessFlags(VkImageLayout layout)
	{
		switch (layout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			return 0;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			return VK_ACCESS_HOST_WRITE_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
			return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			return VK_ACCESS_TRANSFER_WRITE_BIT;
		case VK_IMAGE_LAYOUT_GENERAL:
			assert(false && "Don't know how to get a meaningful VkAccessFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
			return 0;
		default:
			assert(false);
			return 0;
		}
	}

	VkPipelineStageFlags GetPipelineStageFlags(VkImageLayout layout)
	{
		switch (layout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			return VK_PIPELINE_STAGE_HOST_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_PIPELINE_STAGE_TRANSFER_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
			return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
			return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		case VK_IMAGE_LAYOUT_GENERAL:
			assert(false && "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
			return 0;
		default:
			assert(false);
			return 0;
		}
	}

	// transition image layout
	void TransitionImageLayout(
		VkCommandBuffer commandBuffer, 
		VkImage image, VkFormat format, 
		VkImageLayout oldLayout, VkImageLayout newLayout, 
		VkImageSubresourceRange subresourceRange
	)
	{
		/**
		* vkCmdPipelineBarrier specification
		* 1. command buffer
		* 2. source stage mask - pipeline stage at which the operations occur that should happen before the barrier
		* 3. destination stage mask - pipeline stage at which the operations occur that should happen after the barrier
		* 4. dependency flags - specify how to handle the barrier in terms of memory dependencies
		* 5. memory barrier count, memory barriers - reference to memory barriers
		* 6. buffer memory barrier count, buffer memory barriers - reference to buffer memory barriers
		* 7. image memory barrier count, image memory barriers - reference to image memory barriers
		*/
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		sourceStage = GetPipelineStageFlags(oldLayout);
		destinationStage = GetPipelineStageFlags(newLayout);
		barrier.srcAccessMask = GetAccessFlags(oldLayout);
		barrier.dstAccessMask = GetAccessFlags(newLayout);

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
		barrier.image = image;                                           // specify image to transition layout

		// set proper aspect mask based on the layout
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) // check if format has stencil component
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		barrier.subresourceRange.baseMipLevel = subresourceRange.baseMipLevel;
		barrier.subresourceRange.levelCount = subresourceRange.levelCount;
		barrier.subresourceRange.baseArrayLayer = subresourceRange.baseArrayLayer;
		barrier.subresourceRange.layerCount = subresourceRange.layerCount;

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	// transition image layout
	void TransitionImageLayoutVerbose(
		VkCommandBuffer commandBuffer,
		VkImage image, VkFormat format,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask
	)
	{
		/**
		* vkCmdPipelineBarrier specification
		* 1. command buffer
		* 2. source stage mask - pipeline stage at which the operations occur that should happen before the barrier
		* 3. destination stage mask - pipeline stage at which the operations occur that should happen after the barrier
		* 4. dependency flags - specify how to handle the barrier in terms of memory dependencies
		* 5. memory barrier count, memory barriers - reference to memory barriers
		* 6. buffer memory barrier count, buffer memory barriers - reference to buffer memory barriers
		* 7. image memory barrier count, image memory barriers - reference to image memory barriers
		*/
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		sourceStage = srcStage;
		destinationStage = dstStage;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
		barrier.image = image;                                           // specify image to transition layout

		// set proper aspect mask based on the layout
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) // check if format has stencil component
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		barrier.subresourceRange.baseMipLevel = subresourceRange.baseMipLevel;
		barrier.subresourceRange.levelCount = subresourceRange.levelCount;
		barrier.subresourceRange.baseArrayLayer = subresourceRange.baseArrayLayer;
		barrier.subresourceRange.layerCount = subresourceRange.layerCount;

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
} 