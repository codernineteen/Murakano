#pragma once

// internal
#include "Utilities.h"
#include "MKDevice.h"
#include "MKRenderPass.h"

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
	VkFormat		GetSwapchainImageFormat() const { return _vkSwapchainImageFormat; }
	// reusable image view creation
	VkImageView		CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	VkRenderPass	RequestRenderPass() const { return _mkRenderPassPtr->GetRenderPass(); }

private:
	void CreateSwapchainImageViews();
	void CreateFrameBuffers();

private:
	VkSwapchainKHR				_vkSwapchain;
	std::vector<VkImage>		_vkSwapchainImages;
	std::vector<VkImageView>	_vkSwapchainImageViews;
	std::vector<VkFramebuffer>	_vkSwapchainFramebuffers;
	VkFormat					_vkSwapchainImageFormat;
	VkExtent2D					_vkSwapchainExtent;
		
private:
	MKDevice&						_mkDeviceRef;
	std::shared_ptr<MKRenderPass>	_mkRenderPassPtr;
};