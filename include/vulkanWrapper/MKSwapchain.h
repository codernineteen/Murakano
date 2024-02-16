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
	// getters
	VkFormat	   GetSwapchainImageFormat()  const { return _vkSwapchainImageFormat; }
	VkExtent2D	   GetSwapchainExtent()	      const { return _vkSwapchainExtent; }
	VkFramebuffer  GetCurrentFramebuffer()    const { return _vkSwapchainFramebuffers[_currentFrame]; }
	uint32_t	   GetCurrentFrame()          const { return _currentFrame; }
	VkRenderPass   RequestRenderPass()        const { return _mkRenderPassPtr->GetRenderPass(); }

	// reusable image view creation
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
	MKDevice&						_mkDeviceRef;
	std::shared_ptr<MKRenderPass>	_mkRenderPassPtr;
	uint32_t						_currentFrame = 0;
};