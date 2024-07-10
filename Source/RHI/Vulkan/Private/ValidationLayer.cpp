#include "ValidationLayer.h"

MKValidationLayer::MKValidationLayer()
    : _debugMessenger(VK_NULL_HANDLE)
{
#ifndef NDEBUG
    if (!CheckValidationLayerSupport()) 
		MK_THROW("validation layers requested, but not available.");
#endif
}

MKValidationLayer::~MKValidationLayer() {}

bool MKValidationLayer::CheckValidationLayerSupport()
{
    uint32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // get all available layers

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) // if layer name is same as requested
            { 
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void MKValidationLayer::SetupDebugMessenger(VkInstance instance)
{
#ifdef NDEBUG
    return;
#endif

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = vkinfo::GetDebugMessengerCreateInfo(DebugCallback);
    if (CreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr) != VK_SUCCESS) 
        throw std::runtime_error("failed to set up debug messenger");
}

VkResult MKValidationLayer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	auto createProxyFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (createProxyFunc != nullptr) {
        return createProxyFunc(instance, pCreateInfo, pAllocator, &_debugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void MKValidationLayer::DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    auto destroyProxyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyProxyFunc != nullptr) {
        destroyProxyFunc(instance, _debugMessenger, pAllocator);
    }
}
