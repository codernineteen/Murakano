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
	while (width == 0 || height == 0) {
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

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = _mkDeviceRef.GetSurface();
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = actualExtent;
	swapchainCreateInfo.imageArrayLayers = 1;				// always 1 except for stereoscopic 3D application
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly to images

	MKDevice::QueueFamilyIndices indices = _mkDeviceRef.FindQueueFamilies(_mkDeviceRef.GetPhysicalDevice());
	uint32 queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily.value() != indices.presentFamily.value())
	{
		// If graphics family and present family are separate queue, swapchain image shared by multiple queue families
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapchainCreateInfo.preTransform = supportDetails.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE; // ignore the pixels that are obscured by other windows
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

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

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _mkRenderPassPtr->GetRenderPass();
		framebufferInfo.attachmentCount = SafeStaticCast<size_t, uint32>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = _vkSwapchainExtent.width;
		framebufferInfo.height = _vkSwapchainExtent.height;
		framebufferInfo.layers = 1; // number of layers in image arrays

		MK_CHECK(vkCreateFramebuffer(_mkDeviceRef.GetDevice(), &framebufferInfo, nullptr, &_vkSwapchainFramebuffers[i]));
	}
}

