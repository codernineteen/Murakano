#pragma once

// std
#include <map>
#include <optional>

// internal
#include "Utilities.h"
#include "Global.h"
#include "MKInstance.h"


// [MKDevice class]
// - Responsibility :
//	  - responsible for finding a suitable physical device and creating a logical device.
// - Dependency :
//    - MKInstance as reference
//    - MKSwapchain as member variable
class MKDevice 
{
public:
	struct QueueFamilyIndices
	{
		std::optional<uint32> graphicsFamily;
		std::optional<uint32> presentFamily;

		bool isComplete() const
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR			capabilities;	// basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
		std::vector<VkSurfaceFormatKHR>		formats;		// surface formats	(pixel format, color space)
		std::vector<VkPresentModeKHR>		presentModes;	// presentation modes (conditions for "swapping" images to the screen)
	};

public:
	MKDevice(MKWindow& windowRef, const MKInstance& instanceRef);
	~MKDevice();

	/* getters */
	inline VkPhysicalDevice  GetPhysicalDevice()  const { return _vkPhysicalDevice; }
	inline VkDevice		     GetDevice()		  const { return _vkLogicalDevice; }
	inline VkSurfaceKHR	     GetSurface()		  const { return _vkSurface; }
	inline VkQueue			 GetGraphicsQueue()   const { return _vkGraphicsQueue; }
	inline VkQueue			 GetPresentQueue()	  const { return _vkPresentQueue; }
	inline MKWindow&         GetWindowRef()		  const { return _mkWindowRef; }

	/* setters */
	inline void              SetFrameBufferResized(bool isResized) { _mkWindowRef.framebufferResized = isResized; }

	// support checkers
	SwapChainSupportDetails  QuerySwapChainSupport(VkPhysicalDevice device);
	QueueFamilyIndices		 FindQueueFamilies(VkPhysicalDevice device);

	// common apis
	void WaitUntilDeviceIdle() { vkDeviceWaitIdle(_vkLogicalDevice); }
	
	// Belows are related to swapchain creation.
	// I located these functions in Device class for consistency vecause one of them requires window refernce.
	VkSurfaceFormatKHR  ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR	ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D			ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	void  PickPhysicalDevice();
	int	  RateDeviceSuitability(VkPhysicalDevice device);
	bool  IsDeviceExtensionSupported(VkPhysicalDevice device);
	void  CreateWindowSurface();

private:
	VkPhysicalDevice  _vkPhysicalDevice = VK_NULL_HANDLE;		// actual physical device
	VkDevice		  _vkLogicalDevice = VK_NULL_HANDLE;		// an interface to communicate with physical device
	VkSurfaceKHR	  _vkSurface = VK_NULL_HANDLE;			// an interface to communicate with the window system
	VkQueue			  _vkGraphicsQueue;
	VkQueue			  _vkPresentQueue;

private:
	const MKInstance&  _mkInstanceRef;
	MKWindow&	       _mkWindowRef;

public:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME		// macro from VK_KHR_swapchain extension
	};
};