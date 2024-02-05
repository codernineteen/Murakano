#pragma once

// internal
#include <map>
#include <optional>
#include "Utilities.h"
#include "MKInstance.h"


class MKDevice 
{
public:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;		// basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
		std::vector<VkSurfaceFormatKHR> formats;	// surface formats	(pixel format, color space)
		std::vector<VkPresentModeKHR> presentModes; // presentation modes (conditions for "swapping" images to the screen)
	};

public:
	MKDevice(const MKInstance& mkInstance);
	~MKDevice();
	VkDevice GetDevice() const { return _logicalDevice; }

private:
	void PickPhysicalDevice(const MKInstance& mkInstance);
	int RateDeviceSuitability(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	bool isDeviceExtensionSupported(VkPhysicalDevice device);

private:
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _logicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

public:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME // macro from VK_KHR_swapchain extension
	};
};