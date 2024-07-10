#pragma once

// internal
#include "Utilities.h"
#include "Info.h"
#include "MKWindow.h"
#include "MKValidationLayer.h"

// [MKInstance class]
// - Responsibility:
//   - a wrapper for VkInstance
//   - Create and manage a Vulkan instance
// - Dependency:
//   - VkInstance
class MKInstance
{
public:
	MKInstance();
	~MKInstance();

	// getter for _vkInstance access
	VkInstance GetVkInstance() const { return _vkInstance; }

private:
	std::vector<const char*> GetRequiredExtensions();
	std::vector<VkExtensionProperties> GetAvailableExtensions();
	bool CheckExtensionSupport(std::vector<const char*>& requiredExtensions);

private:
	VkInstance			_vkInstance;
	MKValidationLayer	_validationLayer;
};

