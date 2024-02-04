#pragma once

#ifdef DEBUG_MURAKANO
 #define ENABLE_VALIDATION_LAYERS 1
#else
 #define ENABLE_VALIDATION_LAYERS 0
#endif

#include "Utilities.h"

// [MKWindow class]
// - Responsibility :
//    - Vulkan validation layers setup
class MKValidationLayer
{
public:
	MKValidationLayer();
	~MKValidationLayer();

    bool CheckValidationLayerSupport();
    void SetupDebugMessenger(VkInstance instance);
    // get the address of proxy function of vkCreateDebugUtilsMessengerEXT function
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator);
    // reusuable function to populate VkDebugUtilsMessengerCreateInfoEXT struct for instance creation
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo); 
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator);

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(            // VKAPI_ATTR and VKAPI_CALL ensure correct function signature
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     // severity of message. we can use comparision operator to check severity.
        VkDebugUtilsMessageTypeFlagsEXT messageType,                // message types. ex) unrelated to spec , violate spec , potential non-optimal use of api
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  // member of this struct is 'pMessage, pObjects, objectCount'
        void* pUserData                                             // custom data passed from vkCreateDebugUtilsMessengerEXT
    ) 
    {
        std::cerr << "[Validation layer says] : " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

private:
    VkDebugUtilsMessengerEXT _debugMessenger;

public:
    const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
};

