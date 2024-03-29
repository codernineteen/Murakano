#pragma once

#include "Utilities.h"
#include "MKDevice.h"
#include "MKCommandService.h"

class MKOffscreenRender
{
public:
	MKOffscreenRender(MKDevice& mkDeviceRef);
	~MKOffscreenRender();

public:
	void CreateOffscreenRenderResource(VkExtent2D extent);
	void DestroyOffscreenRenderResource();
	void RecreateOffscreenRenderResource(VkExtent2D extent);

private:
	/* color image */
	VkImageAllocated      _vkOffscreenColor;
	VkDescriptorImageInfo _vkOffscreenColorDescriptor;

	/* depth image */
	VkImageAllocated      _vkOffscreenDepth;
	VkDescriptorImageInfo _vkOffscreenDepthDescriptor;

	/* frame buffer */
	VkFramebuffer         _vkOffscreenFrameBuffer = VK_NULL_HANDLE;

	/* render pass */
	VkRenderPass          _vkOffscreenRenderPass = VK_NULL_HANDLE;

private:
	MKDevice&     _mkDeviceRef;
};