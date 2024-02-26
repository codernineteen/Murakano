#include "MKCommandService.h"

MKCommandService::MKCommandService() 
{

}

MKCommandService::~MKCommandService()
{
	vkDestroyCommandPool(_mkDevicePtr->GetDevice(), _vkCommandPool, nullptr);
}

void MKCommandService::InitCommandService(MKDevice* mkDevicePtr)
{
	// assign device pointer
	_mkDevicePtr = mkDevicePtr;
	CreateCommandPool(&_vkCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	CreateCommandBuffers();
}

void MKCommandService::SubmitCommandBufferToQueue(
	uint32 currentFrame, 
	VkSemaphore waitSemaphores[], 
	VkPipelineStageFlags waitStages[], 
	VkSemaphore signalSemaphores[], 
	VkQueue loadedQueue, 
	VkFence fence
)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// specify which semaphores to wait on before execution begins and which semaphores to signal once execution finishes
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	// specify which command buffers to actually submit for execution
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_vkCommandBuffers[currentFrame];
	// specify which semaphores to signal once the command buffer(s) have finished execution
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// submit the command buffer to queueisResized
	if (vkQueueSubmit(loadedQueue, 1, &submitInfo, /*optional fence*/ fence) != VK_SUCCESS) 
		throw std::runtime_error("failed to submit draw command buffer!");
}

void MKCommandService::ResetCommandBuffer(uint32 currentFrame)
{
	vkResetCommandBuffer(_vkCommandBuffers[currentFrame], 0);
}

/**
* ---------------- Private -------------------- 
*/

void MKCommandService::CreateCommandPool(VkCommandPool* commandPoolPtr, VkCommandPoolCreateFlags commandFlag)
{
	// find queue family indices
	MKDevice::QueueFamilyIndices queueFamilyIndices = _mkDevicePtr->FindQueueFamilies(_mkDevicePtr->GetPhysicalDevice());

	// specify command pool
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = commandFlag;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(_mkDevicePtr->GetDevice(), &poolInfo, nullptr, commandPoolPtr) != VK_SUCCESS)
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
	allocInfo.commandBufferCount = SafeStaticCast<size_t, uint32>(_vkCommandBuffers.size());

	if (vkAllocateCommandBuffers(_mkDevicePtr->GetDevice(), &allocInfo, _vkCommandBuffers.data()) != VK_SUCCESS) 
		throw std::runtime_error("failed to allocate command buffers!");
}

void MKCommandService::BeginSingleTimeCommands(VkCommandBuffer& commandBuffer)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _vkCommandPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(_mkDevicePtr->GetDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Let the driver knows that this is single time command submission.

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

void MKCommandService::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_mkDevicePtr->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	// Aslternatively, a fence could be used to wait for the command buffer to complete.
	vkQueueWaitIdle(_mkDevicePtr->GetGraphicsQueue());

	// free the command buffer right away.
	vkFreeCommandBuffers(_mkDevicePtr->GetDevice(), _vkCommandPool, 1, &commandBuffer);
}