#pragma once

#include "Utilities.h"
#include "MKDevice.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class MKCommandService
{
public:
	MKCommandService();
	~MKCommandService();

	/* getter */
	VkCommandBuffer GetCommandBuffer(uint32 currentFrame) { return _vkCommandBuffers[currentFrame]; }

	/* supported service */
	void InitCommandService(MKDevice* mkDeviceRef);                               // initialize command service in Device creation stage
	void SubmitCommandBufferToQueue(                                              // submit command buffer to queue
			uint32 currentFrame, 												  
			VkSemaphore waitSemaphores[],										  
			VkPipelineStageFlags waitStages[],									  
			VkSemaphore signalSemaphores[],										  
			VkQueue loadedQueue, 												  
			VkFence fence = nullptr												  
		 );
	void ResetCommandBuffer(uint32 currentFrame);                                 // reset command buffer for reuse
	void AsyncExecuteCommands(std::queue<VoidLambda>& enqueuedCommands);

public:
	/* template implementations */
	template<typename Func>
	void ExecuteSingleTimeCommands(Func&& lambda)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommands(commandBuffer);
		lambda(commandBuffer);
		EndSingleTimeCommands(commandBuffer);
	}

private:
	void BeginSingleTimeCommands(VkCommandBuffer& commandBuffer);                 // begin single time command buffer
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);                    // end single time command buffer and destroy the buffer right away
	void CreateCommandPool(VkCommandPool* commandPoolPtr, VkCommandPoolCreateFlags commandFlag);
	void CreateCommandBuffers();

private:
	MKDevice*						_mkDevicePtr = nullptr;
	VkCommandPool					_vkCommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer>	_vkCommandBuffers = std::vector<VkCommandBuffer>();
};