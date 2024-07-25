#include "Swapchain.h"

/*
-----------	PUBLIC ------------
*/

MKSwapchain::MKSwapchain(MKDevice& deviceRef) 
	: _mkDeviceRef(deviceRef)
{
	// initialize descriptor manager
	GDescriptorManager->InitDescriptorManager(&_mkDeviceRef);

	CreateSwapchain();

	// create image views mapped to teh swapchain images.
	CreateSwapchainImageViews();

	// create depth resources
	CreateDepthResources();
}

MKSwapchain::~MKSwapchain()
{
	DestroySwapchainResources();

	// destroy global descriptor manager instance
	delete GDescriptorManager;
#ifndef NDEBUG
	MK_LOG("VkFramebuffer destroyed");
	MK_LOG("swapchain image view destroyed");
	MK_LOG("swapchin extension destroyed");
#endif
}

void MKSwapchain::DestroySwapchainResources()
{
    DestroyDepthResources();

	// destroy image views
	for (auto imageView : _vkSwapchainImageViews)
		vkDestroyImageView(_mkDeviceRef.GetDevice(), imageView, nullptr);

	// destroy swapchain extension
	vkDestroySwapchainKHR(_mkDeviceRef.GetDevice(), _vkSwapchain, nullptr);
}

void MKSwapchain::DestroyDepthResources()
{
	vmaDestroyImage(_mkDeviceRef.GetVmaAllocator(), _vkDepthImage.image, _vkDepthImage.allocation);
	vkDestroyImageView(_mkDeviceRef.GetDevice(), _vkDepthImageView, nullptr);
}

void MKSwapchain::CreateSwapchain()
{
	MKDevice::SwapChainSupportDetails supportDetails = _mkDeviceRef.QuerySwapChainSupport(_mkDeviceRef.GetPhysicalDevice());
	VkSurfaceFormatKHR surfaceFormat                 = _mkDeviceRef.ChooseSwapSurfaceFormat(supportDetails.formats);
	VkPresentModeKHR presentMode                     = _mkDeviceRef.ChooseSwapPresentMode(supportDetails.presentModes);
	VkExtent2D actualExtent                          = _mkDeviceRef.ChooseSwapExtent(supportDetails.capabilities);

	uint32 imageCount = supportDetails.capabilities.minImageCount + 1;	// It is recommended to request at least one more image than the minimum
	if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount)
		imageCount = supportDetails.capabilities.maxImageCount;			// clamp to the maximum image count

	// query queue family indices and compare them to check if they are exclusive
	MKDevice::QueueFamilyIndices indices = _mkDeviceRef.FindQueueFamilies(_mkDeviceRef.GetPhysicalDevice());
	uint32 queueFamilyIndices[]          = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	bool isExclusive                     = indices.graphicsFamily.value() == indices.presentFamily.value();

	VkSwapchainCreateInfoKHR swapchainCreateInfo = vkinfo::GetSwapchainCreateInfo(
		_mkDeviceRef.GetSurface(),
		surfaceFormat,
		supportDetails.capabilities,
		presentMode,
		imageCount,
		actualExtent,
		queueFamilyIndices,
		isExclusive
	);

	// create swapchain
	MK_CHECK(vkCreateSwapchainKHR(_mkDeviceRef.GetDevice(), &swapchainCreateInfo, nullptr, &_vkSwapchain));

	// retrieve swapchain images
	vkGetSwapchainImagesKHR(_mkDeviceRef.GetDevice(), _vkSwapchain, &imageCount, nullptr);
	_vkSwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(_mkDeviceRef.GetDevice(), _vkSwapchain, &imageCount, _vkSwapchainImages.data());

	// store swapchain format and extent
	_vkSwapchainImageFormat = surfaceFormat.format;
	_vkSwapchainExtent = actualExtent;
}

void MKSwapchain::CreateSwapchainImageViews()
{
	_vkSwapchainImageViews.resize(_vkSwapchainImages.size());
	for (size_t i = 0; i < _vkSwapchainImages.size(); i++)
	{
		mk::vk::CreateImageView(
			_mkDeviceRef.GetDevice(), 
			_vkSwapchainImages[i],
			_vkSwapchainImageViews[i],
			_vkSwapchainImageFormat, 
			VK_IMAGE_ASPECT_COLOR_BIT, 
			1
		);
	}
}

void MKSwapchain::CreateDepthResources()
{
	// find depth format first
	VkFormat depthFormat = mk::vk::FindDepthFormat(_mkDeviceRef.GetPhysicalDevice());

	// create depth image
	mk::vk::CreateImage(
		_mkDeviceRef.GetVmaAllocator(),
		_vkDepthImage,
		_vkSwapchainExtent.width,
		_vkSwapchainExtent.height,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);

	// create depth image view
	mk::vk::CreateImageView(
		_mkDeviceRef.GetDevice(),
		_vkDepthImage.image,
		_vkDepthImageView,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT, // set DEPTH aspect flags
		1
	);
}

