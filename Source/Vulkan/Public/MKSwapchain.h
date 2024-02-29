#pragma once

// internal
#include "Utilities.h"
#include "MKDevice.h"
#include "MKRenderPass.h"
#include "MKCommandService.h"
#include "MKDescriptorManager.h"

// [MKSwapchain class]
// - Responsibility :
//    - swapchain and image management.
// - Dependency :
//    - MkDevice as refernece
class MKSwapchain
{
public:
	MKSwapchain(MKDevice& deviceWrapper);
	~MKSwapchain();

public:
	/* getters */
	VkSwapchainKHR GetSwapchain()			           const { return _vkSwapchain; }
	VkFormat	   GetSwapchainImageFormat()           const { return _vkSwapchainImageFormat; }
	VkExtent2D	   GetSwapchainExtent()	               const { return _vkSwapchainExtent; }
	VkFramebuffer  GetFramebuffer(uint32 imageIndex)   const { return _vkSwapchainFramebuffers[imageIndex]; }
	VkRenderPass   RequestRenderPass()                 const { return _mkRenderPassPtr->GetRenderPass(); }

	/* setters */
	void		   RequestFramebufferResize(bool isResized) { _mkDeviceRef.SetFrameBufferResized(isResized); }
	void           DestroySwapchainResources();
	void           RecreateSwapchain();

private:
	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void CreateFrameBuffers();

private:
	VkSwapchainKHR				_vkSwapchain;
	std::vector<VkImage>		_vkSwapchainImages;
	std::vector<VkImageView>	_vkSwapchainImageViews;
	std::vector<VkFramebuffer>  _vkSwapchainFramebuffers;
	VkFormat					_vkSwapchainImageFormat;
	VkExtent2D					_vkSwapchainExtent;
		
private:
	MKDevice&					   _mkDeviceRef;
	std::shared_ptr<MKRenderPass>  _mkRenderPassPtr;
};