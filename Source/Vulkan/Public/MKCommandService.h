#pragma once

#include "Utilities.h"
#include "MKDevice.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class MKCommandService
{
public:
	/* constructor , destructor */
	MKCommandService();
	~MKCommandService();

	/* getter */
	VkCommandBuffer GetCommandBuffer(uint32 currentFrame) { return _vkCommandBuffers[currentFrame]; }

	/* supported service */
	void InitCommandService(MKDevice* mkDeviceRef);
	void BeginSingleTimeCommands(VkCommandBuffer& commandBuffer);
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void SubmitCommandBufferToQueue(
			uint32 currentFrame, 
			VkSemaphore waitSemaphores[],
			VkPipelineStageFlags waitStages[],
			VkSemaphore signalSemaphores[],
			VkQueue loadedQueue, 
			VkFence fence = nullptr
		 );
	void ResetCommandBuffer(uint32 currentFrame);

private:
	void CreateCommandPool(VkCommandPool* commandPoolPtr, VkCommandPoolCreateFlags commandFlag);
	void CreateCommandBuffers();

private:
	MKDevice*						_mkDevicePtr			= nullptr;
	VkCommandPool					_vkCommandPool			= VK_NULL_HANDLE;
	std::vector<VkCommandBuffer>	_vkCommandBuffers		= std::vector<VkCommandBuffer>();
};