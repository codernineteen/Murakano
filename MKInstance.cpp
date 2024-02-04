#include "MKInstance.h"

MKInstance::MKInstance()
{
    // specify application create info
	VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // set app info enum variant to struct Type field
    appInfo.pApplicationName = "Murakano"; // application name
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // version specifier
    appInfo.pEngineName = "NO ENGINE";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3; // Murakano using v1.3.268 vulkan api.

    auto extension = GetRequiredExtensions();

    // specify instance create info
	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extension.size());
    instanceInfo.ppEnabledExtensionNames = extension.data();

    // create VkInstance
    if(vkCreateInstance(&instanceInfo, nullptr, &_vkInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create instance");
    }
}

MKInstance::~MKInstance()
{
    vkDestroyInstance(_vkInstance, nullptr);
    std::cout << "MKInstance and VkInstance resources destroyed" << std::endl;
}

std::vector<const char*> MKInstance::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (!CheckExtensionSupport(extensions)) 
    {
        throw std::runtime_error("Required extensions are not supported");
    }

    // TODO : add validation

    return extensions;
}

std::vector<VkExtensionProperties> MKInstance::GetAvailableExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    for (const auto& e : extensions)
        std::cout << "\t " << e.extensionName << std::endl;
   
    return extensions;
}

bool MKInstance::CheckExtensionSupport(std::vector<const char*>& requiredExtensions)
{
    auto availableExtensions = GetAvailableExtensions();
    auto requiredSize = requiredExtensions.size();

    // check if required extensions are available
    // if avaialbe, decrement requiredSize
    for (const auto& availableExtension : availableExtensions) 
    {
        for (const auto& requiredExtenion : requiredExtensions) {
			if (strcmp(availableExtension.extensionName, requiredExtenion) == 0) {
				requiredSize--;
				break;
			}
        }
    }

    return requiredSize == 0;
}
