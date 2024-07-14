#pragma once

// internal
#include "Utilities.h"
#include "Device.h"
#include "CommandService.h"
#include "DescriptorManager.h"

class MKSwapchain
{
public:
	MKSwapchain(MKDevice& deviceWrapper);
	~MKSwapchain();

public:
	/* getters */
	VkSwapchainKHR GetSwapchain()			            const { return _vkSwapchain; }
	VkFormat       GetSwapchainImageFormat()            const { return _vkSwapchainImageFormat; }
	VkExtent2D     GetSwapchainExtent()	                const { return _vkSwapchainExtent; }
	VkImage        GetSwapchainImage(uint32 imageIndex) const { return _vkSwapchainImages[imageIndex]; }
	VkFramebuffer  GetFramebuffer(uint32 imageIndex)    const { return _vkSwapchainFramebuffers[imageIndex]; }

	/* setters */
	void RequestFramebufferResize(bool isResized) { _mkDeviceRef.SetFrameBufferResized(isResized); }
	void DestroySwapchainResources();
	void DestroyDepthResources();

	/* api */
	void CreateFrameBuffers(VkRenderPass renderPass);
	void RecreateSwapchain(VkRenderPass renderPass);

private:
	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void CreateDepthResources();

private:
	VkSwapchainKHR				_vkSwapchain;
	std::vector<VkImage>		_vkSwapchainImages;
	std::vector<VkImageView>	_vkSwapchainImageViews;
	std::vector<VkFramebuffer>  _vkSwapchainFramebuffers;
	VkFormat					_vkSwapchainImageFormat;
	VkExtent2D					_vkSwapchainExtent;
	VkImageAllocated            _vkDepthImage;
	VkImageView                 _vkDepthImageView;

private:
	MKDevice& _mkDeviceRef;
};