#include "MKDevice.h"

MKDevice::MKDevice(const MKWindow& windowRef,const MKInstance& instanceRef)
	: _mkWindowRef(windowRef), _mkInstanceRef(instanceRef)
{
	CreateWindowSurface();
	PickPhysicalDevice();

	QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f; // priority of the queue for scheduling purposes . ranged from 0.0 to 1.0
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;		// index of the queue family to create
		queueCreateInfo.queueCount = 1;						// number of queues to create
		queueCreateInfo.pQueuePriorities = &queuePriority;	// array of queue priorities
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// TODO : specify the set of device features
	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();								// queue create info
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());		// number of queue create infos
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;										// device features
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());	// number of device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();							// device extensions
	if (ENABLE_VALIDATION_LAYERS) 
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;													// number of layers
		deviceCreateInfo.ppEnabledLayerNames = nullptr;											// layers
	}

	if (vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_logicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	// retrieve queue handle from logical device
	vkGetDeviceQueue(_logicalDevice, indices.graphicsFamily.value(), 0, &_graphicsQueue);
	vkGetDeviceQueue(_logicalDevice, indices.presentFamily.value(), 0, &_presentQueue);
}

MKDevice::~MKDevice()
{
	vkDestroySurfaceKHR(_mkInstanceRef.GetVkInstance(), _surface, nullptr);
	vkDestroyDevice(_logicalDevice, nullptr);
}

void MKDevice::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
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
	{
		_physicalDevice = candidates.rbegin()->second;
	}
	else
	{
		throw std::runtime_error("failed to find a suitable GPU");	// if highest score is 0, throw exception
	}
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

	if(!indices.isComplete() || !extensionsSupported || details.formats.empty() || details.presentModes.empty())
		score = 0;

	return score;
}

MKDevice::QueueFamilyIndices MKDevice::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// fill out queue families vector.
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;													// index for each queue family in whole queue families
	for (const auto& queueFamily : queueFamilies) 
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {	// check if queue family supports graphics
			indices.graphicsFamily = i;							// assign index to 'graphics family'
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
		
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

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);			// get surface capabilities

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);				// get surface formats
	if (formatCount != 0) {
		details.formats.resize(formatCount);										
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
	}
	
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);	// get present modes

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool MKDevice::IsDeviceExtensionSupported(VkPhysicalDevice device)
{
	uint32_t availableExtensionCount;
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
	if (glfwCreateWindowSurface(_mkInstanceRef.GetVkInstance(), _mkWindowRef.GetWindow(), nullptr, &_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
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
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
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
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}
