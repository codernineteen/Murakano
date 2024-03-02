#include "MKDevice.h"
#include "MKCommandService.h"

MKDevice::MKDevice(MKWindow& windowRef,const MKInstance& instanceRef)
	: _mkWindowRef(windowRef), _mkInstanceRef(instanceRef)
{

	CreateWindowSurface();
	PickPhysicalDevice();

	QueueFamilyIndices indices = FindQueueFamilies(_vkPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;                             // priority of the queue for scheduling purposes . ranged from 0.0 to 1.0
	for (uint32 queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;		// index of the queue family to create
		queueCreateInfo.queueCount = 1;						// number of queues to create
		queueCreateInfo.pQueuePriorities = &queuePriority;	// array of queue priorities
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// specify the set of device features
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE; // enable anisotropic filtering

	// specify device creation info
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();								       // queue create info
	deviceCreateInfo.queueCreateInfoCount = SafeStaticCast<size_t, uint32>(queueCreateInfos.size());   // number of queue create infos
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;										       // device features
	deviceCreateInfo.enabledExtensionCount = SafeStaticCast<size_t, uint32>(deviceExtensions.size());  // number of device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();							       // device extensions
#ifdef NDEBUG
	deviceCreateInfo.enabledLayerCount = 0;													        // number of layers
	deviceCreateInfo.ppEnabledLayerNames = nullptr;											        // layers
#else
	deviceCreateInfo.enabledLayerCount = SafeStaticCast<size_t, uint32>(validationLayers.size());
	deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#endif
	MK_CHECK(vkCreateDevice(_vkPhysicalDevice, &deviceCreateInfo, nullptr, &_vkLogicalDevice));

	// retrieve queue handles from logical device
	vkGetDeviceQueue(_vkLogicalDevice, indices.graphicsFamily.value(), 0, &_vkGraphicsQueue);
	vkGetDeviceQueue(_vkLogicalDevice, indices.presentFamily.value(), 0, &_vkPresentQueue);

	// initialize command service
	GCommandService->InitCommandService(this);
}

MKDevice::~MKDevice()
{
	delete GCommandService; // command service shoule be deleted before destroying logical device.

	vkDestroySurfaceKHR(_mkInstanceRef.GetVkInstance(), _vkSurface, nullptr);
	vkDestroyDevice(_vkLogicalDevice, nullptr);

#ifndef NDEBUG
	MK_LOG("global command service instance destroyed");
	MK_LOG("surface extension destroyed");
	MK_LOG("logical device destroyed");
#endif
}

void MKDevice::PickPhysicalDevice()
{
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(_mkInstanceRef.GetVkInstance(), &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_mkInstanceRef.GetVkInstance(), &deviceCount, devices.data());

	// rate a score for each device and pick the highest score.
	std::multimap<int, VkPhysicalDevice> candidates;
	for (const auto& device : devices)
	{
		int currentDeviceScore = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(currentDeviceScore, device));
	}

	if (candidates.rbegin()->first > 0)								// last key is the highest score
		_vkPhysicalDevice = candidates.rbegin()->second;
	else
		throw std::runtime_error("failed to find a suitable GPU");	// if highest score is 0, throw exception
}

int MKDevice::RateDeviceSuitability(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 100;

	// discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	// Check device extension support, queue families, isDeviceExtensionSupportedswap chain support
	bool extensionsSupported = IsDeviceExtensionSupported(device);
	QueueFamilyIndices indices = FindQueueFamilies(device);
	SwapChainSupportDetails details = QuerySwapChainSupport(device);

	if(
		!indices.isComplete() || 
		!extensionsSupported || 
		details.formats.empty() || 
		details.presentModes.empty() ||
		!deviceFeatures.samplerAnisotropy
	)
		score = 0;

	return score;
}

MKDevice::QueueFamilyIndices MKDevice::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// fill out queue families vector.
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;													// index for each queue family in whole queue families
	for (const auto& queueFamily : queueFamilies) 
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)     // check if queue family supports graphics
			indices.graphicsFamily = i;							// assign index to 'graphics family'

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _vkSurface, &presentSupport);
		
		// assign index to 'present family'
		if (presentSupport) 
			indices.presentFamily = i;							
		
		// early exit if we find a queue family that supports graphics and present queue.
		if (indices.isComplete()) 
			break;

		i++;
	}

	return indices;
}

MKDevice::SwapChainSupportDetails MKDevice::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _vkSurface, &details.capabilities);			// get surface capabilities

	uint32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vkSurface, &formatCount, nullptr);				// get surface formats
	if (formatCount != 0) {
		details.formats.resize(formatCount);										
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _vkSurface, &formatCount, details.formats.data());
	}
	
	uint32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vkSurface, &presentModeCount, nullptr);	// get present modes

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _vkSurface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool MKDevice::IsDeviceExtensionSupported(VkPhysicalDevice device)
{
	uint32 availableExtensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) 
		requiredExtensions.erase(extension.extensionName); // erase extension name from set

	return requiredExtensions.empty();
}

void MKDevice::CreateWindowSurface()
{
	MK_CHECK(glfwCreateWindowSurface(_mkInstanceRef.GetVkInstance(), _mkWindowRef.GetWindow(), nullptr, &_vkSurface));
}

VkSurfaceFormatKHR MKDevice::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) 
	{
		// target format		: VK_FORMAT_B8G8R8A8_SRGB (32 bits per pixel)
		// target color space	: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

VkPresentModeKHR MKDevice::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		// target present mode			: VK_PRESENT_MODE_MAILBOX_KHR (triple buffering)
		// VK_PRESENT_MODE_MAILBOX_KHR	: presentation engine waits for the next vertical blanking period to update image.(no tearing can be observed.)
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
			return availablePresentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be available
}

VkExtent2D MKDevice::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{	
	if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max()) 
	{
		return capabilities.currentExtent;
	}
	else
	{
		// To avoid resolution mismatch between screen coordinates and the window in pixel,
		// We need to clamp the resolution not to exceed the max and min extent.
		int width, height;
		glfwGetFramebufferSize(_mkWindowRef.GetWindow(), &width, &height);

		VkExtent2D actualExtent = {
			SafeStaticCast<int, uint32>(width),
			SafeStaticCast<int, uint32>(height)
		};
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}
