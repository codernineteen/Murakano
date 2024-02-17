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
	VkCommandBuffer GetCommandBuffer(uint32_t currentFrame) { return _vkCommandBuffers[currentFrame]; }

	/* supported service */
	void InitCommandService(MKDevice* mkDeviceRef);
	void SubmitCommandBufferToQueue(
			uint32_t currentFrame, 
			VkSemaphore waitSemaphores[],
			VkPipelineStageFlags waitStages[],
			VkSemaphore signalSemaphores[],
			VkQueue loadedQueue, 
			VkFence fence = nullptr
		 );
	void ResetCommandBuffer(uint32_t currentFrame);

private:
	void CreateCommandPool(VkCommandPool* commandPoolPtr, VkCommandPoolCreateFlags commandFlag);
	void CreateCommandBuffers();

private:
	MKDevice*						_mkDevicePtr			= nullptr;
	VkCommandPool					_vkCommandPool			= VK_NULL_HANDLE;
	VkCommandPool					_vkTransferCommandPool	= VK_NULL_HANDLE;
	std::vector<VkCommandBuffer>	_vkCommandBuffers		= std::vector<VkCommandBuffer>();
};