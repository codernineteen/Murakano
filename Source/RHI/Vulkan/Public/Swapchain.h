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
	VkSwapchainKHR GetSwapchain()			                const { return _vkSwapchain; }
	VkFormat       GetSwapchainImageFormat()                const { return _vkSwapchainImageFormat; }
	VkExtent2D     GetSwapchainExtent()	                    const { return _vkSwapchainExtent; }
	VkImage        GetSwapchainImage(uint32 imageIndex)     const { return _vkSwapchainImages[imageIndex]; }
	VkImageView    GetSwapchainImageView(uint32 imageIndex) const { return _vkSwapchainImageViews[imageIndex]; }
	VkImage        GetDepthImage()                          const { return _vkDepthImage.image; }
	VkImageView    GetDepthImageView()                      const { return _vkDepthImageView; }
	VkFormat       GetDepthFormat()                         const { return _vkDepthFormat; }
	size_t         GetImageViewCount()                      const { return _vkSwapchainImageViews.size(); }

	/* setters */
	void DestroySwapchainResources();
	void DestroyDepthResources();

	/* api */
	void CreateSwapchain();
	void CreateSwapchainImageViews();
	void CreateDepthResources();

private:
	VkSwapchainKHR				_vkSwapchain;

	/* color images */
	std::vector<VkImage>		_vkSwapchainImages;
	std::vector<VkImageView>	_vkSwapchainImageViews;
	VkFormat					_vkSwapchainImageFormat;

	/* depth image */
	VkImageAllocated            _vkDepthImage;
	VkImageView                 _vkDepthImageView;
	VkFormat                    _vkDepthFormat;

	/* extent */
	VkExtent2D					_vkSwapchainExtent;

private:
	MKDevice& _mkDeviceRef;
};