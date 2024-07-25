#pragma once

#include "Types.h"
#include "Conversion.h"
#include "Macros.h"
#include "Info.h"

// helper function
namespace mk
{
	namespace file
	{
		/* read local file */
		std::vector<char> ReadFile(const std::string& filename);
	}

	namespace vk
	{
		/* a utility to find a suitable memory type*/
		uint32 FindMemoryType(uint32 typeFilter, VkPhysicalDeviceMemoryProperties deviceMemProperties, VkMemoryPropertyFlags properties);

		/* create shader module and return it */
		VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& code);

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
			VmaAllocationCreateFlags memoryAllocationFlags,
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED
		);

		/* create single image view related to the given image parameter */
		void CreateImageView(
			VkDevice logicalDevice,
			VkImage image,
			VkImageView& imageView,
			VkFormat imageFormat,
			VkImageAspectFlags aspectFlags,
			uint32 mipLevels = 0U,
			uint32 layerCount = 1U
		);

		/* create a sampler */
		void CreateSampler(
			VkDevice   logicalDevice,
			VkSampler* sampler,
			VkPhysicalDeviceProperties deviceProperties
		);

		/* image resource destroyer */
		void DestroyImageResource(const VmaAllocator& allocator, const VkDevice& device, VkImageAllocated& imageAllocated, VkImageView& imageView);

		/* find supported device format */
		VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		/* find depth format */
		VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

		/* find depth stencil format */
		VkFormat FindDepthStencilFormat(VkPhysicalDevice physicalDevice);

		/* attain suitable stage flags for given layout */
		VkPipelineStageFlags GetPipelineStageFlags(VkImageLayout layout);

		/* attain suitable access flags for given layout */
		VkAccessFlags GetAccessFlags(VkImageLayout layout);

		/* transition image layout */
		void TransitionImageLayout(
			VkCommandBuffer commandBuffer,
			VkImage image, VkFormat format,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkImageSubresourceRange subresourceRange
		);
		void TransitionImageLayoutVerbose(
			VkCommandBuffer commandBuffer,
			VkImage image, VkFormat format,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask
		);

		/* copy */
		void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);
		


		/* create a buffer */
		VkBufferAllocated CreateBuffer(
			const VmaAllocator& allocator,
			VkDeviceSize size,
			VkBufferUsageFlags bufferUsage,
			VmaMemoryUsage memoryUsage,
			VmaAllocationCreateFlags memoryAllocationFlags
		);

		/* copy resources */
		void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		void CopyBufferToBuffer(VkBufferAllocated src, VkBufferAllocated dest, VkDeviceSize size);
	}
}