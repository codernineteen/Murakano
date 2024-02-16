#include "MKCommandService.h"

MKCommandService::MKCommandService() 
{

}

MKCommandService::~MKCommandService()
{
	vkDestroyCommandPool(_mkDevicePtr->GetDevice(), _vkCommandPool, nullptr);
	vkDestroyCommandPool(_mkDevicePtr->GetDevice(), _vkTransferCommandPool, nullptr);
}

void MKCommandService::InitCommandService(MKDevice* mkDevicePtr)
{
	// assign device pointer
	_mkDevicePtr = mkDevicePtr;
	CreateCommandPool(_vkCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	CreateCommandPool(_vkTransferCommandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	CreateCommandBuffers();

}

void MKCommandService::CreateCommandPool(VkCommandPool commandPool, VkCommandPoolCreateFlags commandFlag)
{
	// find queue family indices
	MKDevice::QueueFamilyIndices queueFamilyIndices = _mkDevicePtr->FindQueueFamilies(_mkDevicePtr->GetPhysicalDevice());

	// specify command pool
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = commandFlag;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(_mkDevicePtr->GetDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

void MKCommandService::CreateCommandBuffers()
{
	// Note that the command buffer is allocated from the command pool and destoryed when the pool is destroyed.
	_vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(_vkCommandBuffers.size());

	if (vkAllocateCommandBuffers(_mkDevicePtr->GetDevice(), &allocInfo, _vkCommandBuffers.data()) != VK_SUCCESS) 
		throw std::runtime_error("failed to allocate command buffers!");
}

