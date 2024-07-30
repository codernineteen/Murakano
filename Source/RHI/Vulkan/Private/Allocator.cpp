#include "Allocator.h"

Allocator::Allocator() {}
Allocator::~Allocator() 
{
	vmaDestroyAllocator(_vmaAllocator);
}

/*
----------- Initializer -----------
*/
void Allocator::InitVMAAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &_vmaAllocator);
}


/*
----------- Allocation -----------
*/
void Allocator::CreateBuffer(
	VkBufferAllocated*       newBuffer,
	VkDeviceSize             size,
	VkBufferUsageFlags       bufferUsage,
	VmaMemoryUsage           memoryUsage,
	VmaAllocationCreateFlags memoryAllocationFlags,
	std::string              allocationName
)
{
	// specify buffer creation info
	VkBufferCreateInfo bufferInfo = mk::vkinfo::GetBufferCreateInfo(size, bufferUsage, VK_SHARING_MODE_EXCLUSIVE);

	VmaAllocationCreateInfo bufferAllocInfo{};
	bufferAllocInfo.usage = memoryUsage;
	bufferAllocInfo.flags = memoryAllocationFlags;

	// create buffer
	newBuffer->name = allocationName;
	MK_CHECK(vmaCreateBuffer(_vmaAllocator, &bufferInfo, &bufferAllocInfo, &newBuffer->buffer, &newBuffer->allocation, &newBuffer->allocationInfo));
}

void Allocator::CreateImage(
	VkImageAllocated*        newImage,
	uint32                   width,
	uint32                   height,
	VkFormat                 format,
	VkImageTiling            tiling,
	VkImageUsageFlags        usage,
	VmaMemoryUsage           memoryUsage,
	VmaAllocationCreateFlags memoryAllocationFlags,
	VkImageLayout            layout,
	std::string              allocationName
)
{
	// specify image creation info
	VkImageCreateInfo imageInfo = mk::vkinfo::GetImageCreateInfo(width, height, format, tiling, usage, layout);

	VmaAllocationCreateInfo imageAllocInfo{};
	imageAllocInfo.usage = memoryUsage;
	imageAllocInfo.flags = memoryAllocationFlags;

	// create image
	newImage->name = allocationName;
	MK_CHECK(vmaCreateImage(_vmaAllocator, &imageInfo, &imageAllocInfo, &newImage->image, &newImage->allocation, &newImage->allocationInfo));
}

/*
----------- Destruction -----------
*/
void Allocator::DestroyBuffer(VkBufferAllocated& bufferAllocated) const
{
	vmaDestroyBuffer(_vmaAllocator, bufferAllocated.buffer, bufferAllocated.allocation);
#ifndef NDEBUG
	std::string msg = "Destroying buffer with allocation name : " + bufferAllocated.name;
	MK_LOG(msg);
#endif
}

void Allocator::DestroyImage(VkImageAllocated& imageAllocated) const
{
	vmaDestroyImage(_vmaAllocator, imageAllocated.image, imageAllocated.allocation);
#ifndef NDEBUG
	std::string msg = "Destroying image with allocation name : " + imageAllocated.name;
	MK_LOG(msg);
#endif
}