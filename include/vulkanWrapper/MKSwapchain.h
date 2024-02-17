#pragma once

// internal
#include "Utilities.h"
#include "MKDevice.h"
#include "MKRenderPass.h"
#include "MKCommandService.h"

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
	VkSwapchainKHR  GetSwapchain()			            const { return _vkSwapchain; }
	VkFormat	    GetSwapchainImageFormat()           const { return _vkSwapchainImageFormat; }
	VkExtent2D	    GetSwapchainExtent()	            const { return _vkSwapchainExtent; }
	VkFramebuffer   GetFramebuffer(uint32_t imageIndex) const { return _vkSwapchainFramebuffers[imageIndex]; }
	VkRenderPass    RequestRenderPass()                 const { return _mkRenderPassPtr->GetRenderPass(); }

	/* setters */
	void		   SetFrameBufferResized(bool isResized) { _framebufferResized = isResized; }

	/* reusuable image creation */
	VkImageView    CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

private:
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
	bool						   _framebufferResized = false;
};