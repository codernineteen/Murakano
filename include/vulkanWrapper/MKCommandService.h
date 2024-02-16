#pragma once

#define MAX_FRAMES_IN_FLIGHT 2

#include "Utilities.h"
#include "MKDevice.h"

class MKCommandService
{
public:
	MKCommandService();
	~MKCommandService();

	void			InitCommandService(MKDevice* mkDeviceRef);
	VkCommandBuffer GetCommandBuffer(uint32_t currentFrame) { return _vkCommandBuffers[currentFrame]; }

private:
	void CreateCommandPool(VkCommandPool commandPool, VkCommandPoolCreateFlags commandFlag);
	void CreateCommandBuffers();

private:
	MKDevice*						_mkDevicePtr			= nullptr;
	VkCommandPool					_vkCommandPool			= VK_NULL_HANDLE;
	VkCommandPool					_vkTransferCommandPool	= VK_NULL_HANDLE;
	std::vector<VkCommandBuffer>	_vkCommandBuffers		= std::vector<VkCommandBuffer>();
};