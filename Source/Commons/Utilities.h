#pragma once

// vulkan
#include <vulkan/vulkan.h>

// std
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <memory> // for smart pointers
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

    static void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
		// specify buffer creation info
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;                                   // size of the buffer in bytes
		bufferInfo.usage = bufferUsage;                           // indicate the purpose of the data in the buffer
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;       // buffer will only be used by the graphics queue family

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");

		// query memory requirements for the buffer
		/**
		* VKMemoryRequirements specification
		* 1. size : the size of the required amount of memory in bytes
		* 2. alignment : the offset in bytes where the buffer begins in the allocated region of memory
		* 3. memoryTypeBits : a bitmask and contains one bit set for every supported memory type for the resource. 'Bit i' is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties structure for the physical device is supported for the resource.
		*/
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
		/**
		* VKPhysicalDeviceMemoryProperties specification
		* 1. memoryTypes : different types of memory like device local, host visible, coherent, and cached
		* 2. memoryHeaps : distinct memory resources like dedicated VRAM and swap space in RAM for example (this can affect performance)
		*/
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		uint32 memoryTypeIndex = -1;
		for (uint32 i = 0; i < memProperties.memoryTypeCount; i++)
		{
			// 1. memoryTypeBits is a one-bit bitmask and we can find a match by iterating over each type bit and checking if it is set in the typeFilter
			// 2. If matched index's memory type has all the properties we need, then return the index.
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				memoryTypeIndex = i;
				break;
			}
		}
		if(memoryTypeIndex == -1)
			throw std::runtime_error("failed to find suitable memory type!");	

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;



		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate buffer memory!");

		// Associate allocated memory with the buffer by calling 'vkBindBufferMemory'
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }
}
