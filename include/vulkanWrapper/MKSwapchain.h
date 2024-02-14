#pragma once

// internal
#include "Utilities.h"
#include "MKDevice.h"

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
	VkFormat GetSwapchainImageFormat() const { return _vkSwapchainImageFormat; }
	// reusable image view creation
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

private:
	void CreateSwapchainImageViews();

private:
	VkSwapchainKHR				_vkSwapchain;
	std::vector<VkImage>		_vkSwapchainImages;
	std::vector<VkImageView>	_vkSwapchainImageViews;
	VkFormat					_vkSwapchainImageFormat;
	VkExtent2D					_vkSwapchainExtent;
		
private:
	MKDevice& _deviceRef;

};