#pragma once

#include "Utilities.h"
#include "Device.h"

class MKRenderPass
{
public:
	MKRenderPass(const MKDevice& mkDeviceRef, VkFormat swapchainImageFormat);
	~MKRenderPass();
	
	// getter
	VkRenderPass GetRenderPass() const { return _vkRenderPass; }

private:
	VkRenderPass		_vkRenderPass;

private:
	const MKDevice&		_mkDeviceRef;
};

