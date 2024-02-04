#pragma once

#ifdef DEBUG_MURAKANO
 #define ENABLE_VALIDATION_LAYERS 1
#else
 #define ENABLE_VALIDATION_LAYERS 0
#endif

#include "Utilities.h"
#include "MKInstance.h"

// [MKWindow class]
// - Responsibility :
//    - Vulkan validation layers setup
class MKValidationLayer
{
public:
	MKValidationLayer(std::shared_ptr<MKInstance> pMkInstance);
	~MKValidationLayer();

    bool checkValidationLayerSupport();
    void setupDebugMessenger();
    // get the address of proxy function of vkCreateDebugUtilsMessengerEXT function
    VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger
    );
    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(            // VKAPI_ATTR and VKAPI_CALL ensure correct function signature
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
    std::shared_ptr<MKInstance> _pMkInstance;

private:
    const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
};

