// internal
#include "Utilities.h"
#include "MKDevice.h"

class MKSwapchain
{
public:
	MKSwapchain(MKDevice& deviceWrapper);
	~MKSwapchain();

private:
	VkSwapchainKHR _vkSwapchain;

private:
	MKDevice& _deviceRef;

};