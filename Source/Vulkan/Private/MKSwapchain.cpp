#include "MKSwapchain.h"

/*
-----------	PUBLIC ------------
*/

MKSwapchain::MKSwapchain(MKDevice& deviceRef) 
	: _mkDeviceRef(deviceRef)
{
	CreateSwapchain();
	// create render pass as shared resource. (this should be advance before creating framebuffers)
	_mkRenderPassPtr = std::make_shared<MKRenderPass>(_mkDeviceRef, _vkSwapchainImageFormat);
	// create image views mapped to teh swapchain images.
	CreateSwapchainImageViews();
	// initialize descriptor manager
	GDescriptorManager->InitDescriptorManager(&_mkDeviceRef, _vkSwapchainExtent);

	// create framebuffers after creating swap chain image views and depth image view in descriptor manager.
	CreateFrameBuffers();
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
	GDescriptorManager->DestroyDepthResources();

	// destroy framebuffers
	for (auto framebuffer : _vkSwapchainFramebuffers)
		vkDestroyFramebuffer(_mkDeviceRef.GetDevice(), framebuffer, nullptr);

	// destroy image views
	for (auto imageView : _vkSwapchainImageViews)
		vkDestroyImageView(_mkDeviceRef.GetDevice(), imageView, nullptr);

	// destroy swapchain extension
	vkDestroySwapchainKHR(_mkDeviceRef.GetDevice(), _vkSwapchain, nullptr);
}

void MKSwapchain::RecreateSwapchain()
{
	// handling window minimization
	int width = 0, height = 0;
	glfwGetFramebufferSize(_mkDeviceRef.GetWindowRef().GetWindow(), &width, &height);
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(_mkDeviceRef.GetWindowRef().GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(_mkDeviceRef.GetDevice()); // wait until the device is idle

	DestroySwapchainResources();

	// TODO : recreating render pass may be needed later.
	CreateSwapchain();
	CreateSwapchainImageViews();
	GDescriptorManager->UpdateSwapchainExtent(_vkSwapchainExtent);
	GDescriptorManager->CreateDepthResources();
	CreateFrameBuffers();
}

/*
-----------	PRIVATE ------------
*/

void MKSwapchain::CreateSwapchain()
{
	MKDevice::SwapChainSupportDetails supportDetails = _mkDeviceRef.QuerySwapChainSupport(_mkDeviceRef.GetPhysicalDevice());
	VkSurfaceFormatKHR surfaceFormat = _mkDeviceRef.ChooseSwapSurfaceFormat(supportDetails.formats);
	VkPresentModeKHR presentMode = _mkDeviceRef.ChooseSwapPresentMode(supportDetails.presentModes);
	VkExtent2D actualExtent = _mkDeviceRef.ChooseSwapExtent(supportDetails.capabilities);

	uint32 imageCount = supportDetails.capabilities.minImageCount + 1;	// It is recommended to request at least one more image than the minimum
	if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount)
		imageCount = supportDetails.capabilities.maxImageCount;			// clamp to the maximum image count

	// query queue family indices and compare them to check if they are exclusive
	MKDevice::QueueFamilyIndices indices = _mkDeviceRef.FindQueueFamilies(_mkDeviceRef.GetPhysicalDevice());
	uint32 queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	bool isExclusive = indices.graphicsFamily.value() == indices.presentFamily.value();

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
		util::CreateImageView(
			_mkDeviceRef.GetDevice(), 
			_vkSwapchainImages[i],
			_vkSwapchainImageViews[i],
			_vkSwapchainImageFormat, 
			VK_IMAGE_ASPECT_COLOR_BIT, 
			1
		);
	}
}

void MKSwapchain::CreateFrameBuffers()
{
	_vkSwapchainFramebuffers.resize(_vkSwapchainImageViews.size()); // resize to size of swap chain image views

	// create frame buffer as much as the number of image views
	for (size_t i = 0; i < _vkSwapchainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			_vkSwapchainImageViews[i],
			GDescriptorManager->GetDepthImageView()
		};

		VkFramebufferCreateInfo framebufferInfo = vkinfo::GetFramebufferCreateInfo(_mkRenderPassPtr->GetRenderPass(), attachments, _vkSwapchainExtent);
		MK_CHECK(vkCreateFramebuffer(_mkDeviceRef.GetDevice(), &framebufferInfo, nullptr, &_vkSwapchainFramebuffers[i]));
	}
}

