#include "CommandService.h"

MKCommandService::MKCommandService() 
{

}

MKCommandService::~MKCommandService()
{
	vkDestroyCommandPool(_mkDevicePtr->GetDevice(), _vkCommandPool, nullptr);

#ifndef NDEBUG
	MK_LOG("command pool destroyed");
#endif
}

void MKCommandService::InitCommandService(MKDevice* mkDevicePtr)
{
	// assign device pointer
	_mkDevicePtr = mkDevicePtr;
	/**
	* To reuse same identical command buffer several times, set this flag.
	* Also, enable to reset individual command buffer.
	*/
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
	MK_CHECK(vkQueueSubmit(loadedQueue, 1, &submitInfo, /*optional fence*/ fence));
}

void MKCommandService::ResetCommandBuffer(uint32 currentFrame)
{
	MK_CHECK(vkResetCommandBuffer(_vkCommandBuffers[currentFrame], 0));
}

void MKCommandService::ExecuteCommands(std::queue<VoidLambda>& commandQueue)
{
	VkCommandBuffer commandBuffer;
	BeginSingleTimeCommands(commandBuffer);
	while (!commandQueue.empty())
	{
		commandQueue.front()(commandBuffer);
		commandQueue.pop();
	}
	EndSingleTimeCommands(commandBuffer);
}

void MKCommandService::CreateCommandPool(VkCommandPool* commandPoolPtr, VkCommandPoolCreateFlags commandFlag)
{
	// find queue family indices
	MKDevice::QueueFamilyIndices queueFamilyIndices = _mkDevicePtr->FindQueueFamilies(_mkDevicePtr->GetPhysicalDevice());

	// specify command pool
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = commandFlag;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	MK_CHECK(vkCreateCommandPool(_mkDevicePtr->GetDevice(), &poolInfo, nullptr, commandPoolPtr));
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

	MK_CHECK(vkAllocateCommandBuffers(_mkDevicePtr->GetDevice(), &allocInfo, _vkCommandBuffers.data()));
}

void MKCommandService::BeginSingleTimeCommands(VkCommandBuffer& commandBuffer, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	if (commandPool == VK_NULL_HANDLE)
		allocInfo.commandPool = _vkCommandPool;
	else
		allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	MK_CHECK(vkAllocateCommandBuffers(_mkDevicePtr->GetDevice(), &allocInfo, &commandBuffer));

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Let the driver knows that this is single time command submission.

	MK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

void MKCommandService::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool)
{
	MK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	MK_CHECK(vkQueueSubmit(_mkDevicePtr->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	// Alternatively, a fence could be used to wait for the command buffer to complete.
	MK_CHECK(vkQueueWaitIdle(_mkDevicePtr->GetGraphicsQueue()));

	// free the command buffer right away.
	if(commandPool == VK_NULL_HANDLE)
		vkFreeCommandBuffers(_mkDevicePtr->GetDevice(), _vkCommandPool, 1, &commandBuffer);
	else
	{
		vkFreeCommandBuffers(_mkDevicePtr->GetDevice(), commandPool, 1, &commandBuffer);
		vkDestroyCommandPool(_mkDevicePtr->GetDevice(), commandPool, nullptr);
	}
}