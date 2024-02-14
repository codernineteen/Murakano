#include "MKSwapchain.h"


MKSwapchain::MKSwapchain(MKDevice& deviceRef) 
	: _deviceRef(deviceRef)
{
	MKDevice::SwapChainSupportDetails supportDetails = _deviceRef.QuerySwapChainSupport(_deviceRef.GetPhysicalDevice());
	VkSurfaceFormatKHR surfaceFormat = _deviceRef.ChooseSwapSurfaceFormat(supportDetails.formats);
	VkPresentModeKHR presentMode = _deviceRef.ChooseSwapPresentMode(supportDetails.presentModes);
	VkExtent2D actualExtent = _deviceRef.ChooseSwapExtent(supportDetails.capabilities);

	uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;	// It is recommended to request at least one more image than the minimum
	if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount)
		imageCount = supportDetails.capabilities.maxImageCount;			// clamp to the maximum image count

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = _deviceRef.GetSurface();
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = actualExtent;
	swapchainCreateInfo.imageArrayLayers = 1;				// always 1 except for stereoscopic 3D application
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly to images

	MKDevice::QueueFamilyIndices indices = _deviceRef.FindQueueFamilies(_deviceRef.GetPhysicalDevice());
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

	if (vkCreateSwapchainKHR(_deviceRef.GetDevice(), &swapchainCreateInfo, nullptr, &_vkSwapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create swapchain");

	// retrieve swapchain images
	vkGetSwapchainImagesKHR(_deviceRef.GetDevice(), _vkSwapchain, &imageCount, nullptr);
	_vkSwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(_deviceRef.GetDevice(), _vkSwapchain, &imageCount, _vkSwapchainImages.data());

	// store swapchain format and extent
	_vkSwapchainImageFormat = surfaceFormat.format;
	_vkSwapchainExtent = actualExtent;

	// create image views mapped to teh swapchain images.
	CreateSwapchainImageViews();
}

MKSwapchain::~MKSwapchain()
{
	for (auto imageView : _vkSwapchainImageViews)
		vkDestroyImageView(_deviceRef.GetDevice(), imageView, nullptr);
	vkDestroySwapchainKHR(_deviceRef.GetDevice(), _vkSwapchain, nullptr);
}

VkImageView MKSwapchain::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;			// 2D image in most cases.
	imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;			// first mipmap level accessible to the view
	imageViewCreateInfo.subresourceRange.levelCount = mipLevels;	// number of mipmap levels accessible to the view
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;		// first array layer accessible to the view
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(_deviceRef.GetDevice(), &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image views");

	return imageView;
}

void MKSwapchain::CreateSwapchainImageViews()
{
	_vkSwapchainImageViews.resize(_vkSwapchainImages.size());

	for (size_t i = 0; i < _vkSwapchainImages.size(); i++)
	{
		_vkSwapchainImageViews[i] = CreateImageView(_vkSwapchainImages[i], _vkSwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

