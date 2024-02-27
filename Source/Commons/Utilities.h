#pragma once

// third-party
#include <vulkan/vulkan.h> // vulkan

// std
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>
#include <algorithm>
#include <array>
#include <string>

// internal
#include "Types.h"
#include "Conversion.h"

// helper function
namespace util 
{
	static std::vector<char> ReadFile(const std::string& filename) 
	{
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open() || file.bad()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = static_cast<size_t>(file.tellg()); // advangtage of ios::ate - we can use read position to determine size of file
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);     // read all bytes at once

        file.close();
        return buffer;
	}

	/* a utility to find a suitable memory type*/
	static uint32 FindMemoryType(uint32 typeFilter, VkPhysicalDeviceMemoryProperties deviceMemProperties, VkMemoryPropertyFlags properties) 
	{
		for (uint32 i = 0; i < deviceMemProperties.memoryTypeCount; i++)
		{
			// 1. memoryTypeBits is a one-bit bitmask and we can find a match by iterating over each type bit and checking if it is set in the typeFilter
			// 2. If matched index's memory type has all the properties we need, then return the index.
			if ((typeFilter & (1 << i)) && (deviceMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

    static void CreateBuffer(
		VkPhysicalDevice physicalDevice, 
		VkDevice logicalDevice, 
		VkDeviceSize size, 
		VkBufferUsageFlags bufferUsage, 
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer, 
		VkDeviceMemory& bufferMemory)
    {
		// specify buffer creation info
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;                                   // size of the buffer in bytes
		bufferInfo.usage = bufferUsage;                           // indicate the purpose of the data in the buffer
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;       // buffer will only be used by the graphics queue family

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");

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



		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate buffer memory!");

		// Associate allocated memory with the buffer by calling 'vkBindBufferMemory'
		vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
    }

	static void CreateImage(
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
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;         // multisampling-related
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // If there are more than two queues using the image, then you should use VK_SHARING_MODE_CONCURRENT

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		// query device memory properties
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memProperties, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	static void CreateImageView(
		VkDevice logicalDevice,
		VkImage image, 
		VkImageView& imageView,
		VkFormat imageFormat, 
		VkImageAspectFlags aspectFlags, 
		uint32 mipLevels
	)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;			// 2D image in most cases.
		imageViewCreateInfo.format = imageFormat;						// follw the format of the given swapchain image
		imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;			// first mipmap level accessible to the view
		imageViewCreateInfo.subresourceRange.levelCount = mipLevels;	// number of mipmap levels accessible to the view
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;		// first array layer accessible to the view
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image views");
	}
}
