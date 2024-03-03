#pragma once

#include <vulkan/vulkan.h>
#include "Global.h"
#include "Conversion.h"
#include "Types.h"

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

// the specified fields with hard-coded values are the options for this Murakano renderer implementation.
namespace vkinfo
{
	/* application info */
	VkApplicationInfo                     GetApplicationInfo();
	/* debuge messenger extension info */
	VkDebugUtilsMessengerCreateInfoEXT    GetDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);
	/* vulkan instance create info */
	VkInstanceCreateInfo                  GetInstanceCreateInfo(VkApplicationInfo* appInfo, size_t extensionSize, const char* const* extensionNames, VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo = nullptr);
	/* logical device create info */
	VkDeviceCreateInfo                    GetDeviceCreateInfo(const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos, VkPhysicalDeviceFeatures& deviceFeatures, const std::vector<const char*>& deviceExtensions);
	/* vulkan device queue create info */
	VkDeviceQueueCreateInfo               GetDeviceQueueCreateInfo(uint32 queueFamilyIndex, float queuePriority);
	/* vulkan swapchain create info */
	VkSwapchainCreateInfoKHR              GetSwapchainCreateInfo(
	                                      	 VkSurfaceKHR surface, 
	                                      	 VkSurfaceFormatKHR surfaceFormat, 
	                                      	 VkSurfaceCapabilitiesKHR capabilities, 
	                                      	 VkPresentModeKHR presentMode, 
	                                      	 uint32 imageCount, 
	                                      	 VkExtent2D extent, 
	                                      	 uint32 queueFamilyIndices[], 
	                                      	 bool isExclusive
	                                      );
	/* vulkan frame buffer create info */
	VkFramebufferCreateInfo               GetFramebufferCreateInfo(VkRenderPass renderPass, const std::array<VkImageView, 2>& attachments ,VkExtent2D extent);
	/* vulkan render pass create info */
	VkRenderPassCreateInfo                GetRenderPassCreateInfo(const std::array<VkAttachmentDescription, 2>& attachments, const VkSubpassDescription& subpass, const VkSubpassDependency& dependency);
	/* pipeline shader stage create info */
	VkPipelineShaderStageCreateInfo       GetPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const std::string& entryPoint);
	/* pipeline vertex input state create info */
	template<size_t Size>
	VkPipelineVertexInputStateCreateInfo  GetPipelineVertexInputStateCreateInfo(const VkVertexInputBindingDescription& bindingDescription, const std::array<VkVertexInputAttributeDescription, Size>& attributeDescriptions)
	{
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = SafeStaticCast<size_t, uint32>(Size);
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		return vertexInputInfo;
	}
	/* pipeline input assembly state create info */
	VkPipelineInputAssemblyStateCreateInfo GetPipelineInputAssemblyStateCreateInfo();
	/* pipeline viewport state create info */
	VkPipelineViewportStateCreateInfo      GetPipelineViewportStateCreateInfo();
	/* pipeline dynamic state create info */
	VkPipelineDynamicStateCreateInfo       GetPipelineDynamicStateCreateInfo(const std::vector<VkDynamicState>& dynamicStates);
	/* pipeline rasterization state create info */
	VkPipelineRasterizationStateCreateInfo GetPipelineRasterizationStateCreateInfo();
	/* pipeline multisampling create info */
	VkPipelineMultisampleStateCreateInfo   GetPipelineMultisampleStateCreateInfo();
	/* pipeline depth and stencil state create info */
	VkPipelineDepthStencilStateCreateInfo  GetPipelineDepthStencilStateCreateInfo();
	/* pipeline color blend attachment state */
	VkPipelineColorBlendAttachmentState    GetPipelineColorBlendAttachmentState();
	/* pipeline color blend state create info */
	VkPipelineColorBlendStateCreateInfo    GetPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment);
	/* pipeline layout create info */
	VkPipelineLayoutCreateInfo             GetPipelineLayoutCreateInfo(VkDescriptorSetLayout* descriptorSetLayoutPtr);
	/* graphics pipeline create info */
	VkGraphicsPipelineCreateInfo           GetGraphicsPipelineCreateInfo(
	                                      	 VkPipelineLayout layout,  
	                                      	 VkPipelineShaderStageCreateInfo* shaderStages, 
	                                      	 VkPipelineVertexInputStateCreateInfo* vertexInputInfo, 
	                                      	 VkPipelineInputAssemblyStateCreateInfo* inputAssembly, 
	                                      	 VkPipelineViewportStateCreateInfo* viewportState, 
	                                      	 VkPipelineRasterizationStateCreateInfo* rasterizer, 
	                                      	 VkPipelineMultisampleStateCreateInfo* multisampling, 
	                                      	 VkPipelineDepthStencilStateCreateInfo* depthStencil, 
	                                      	 VkPipelineColorBlendStateCreateInfo* colorBlending, 
	                                      	 VkPipelineDynamicStateCreateInfo* dynamicState,
		                                     const VkPipelineLayout& pipelineLayout,
		                                     const VkRenderPass& renderPass
	                                       );
	/* create image info */
	VkImageCreateInfo                      GetImageCreateInfo(
	                                         uint32 width, 
	                                         uint32 height, 
	                                         VkFormat format, 
	                                         VkImageTiling tiling,
	                                         VkImageUsageFlags usage
	                                       );
	/* create image view info */
	VkImageViewCreateInfo                  GetImageViewCreateInfo(
	                                         VkImage image, 
	                                         VkFormat format, 
	                                         VkImageAspectFlags aspectFlags, 
	                                         uint32 mipLevels
	                                       );
	/* create buffer info */
	VkBufferCreateInfo                     GetBufferCreateInfo(
	                                         VkDeviceSize size, 
	                                         VkBufferUsageFlags usage,
		                                     VkSharingMode sharingMode
	                                       );
};