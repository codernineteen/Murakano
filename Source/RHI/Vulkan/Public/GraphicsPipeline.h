#pragma once

// internal
#include "Utilities.h"
#include "Global.h"
#include "OBJModel.h"
#include "Texture.h"
#include "FreeCamera.h"
#include "InputController.h"

// vulkan wrappers
#include "Device.h"
#include "Swapchain.h"
#include "DescriptorManager.h"
#include "Raytracer.h"

class MKGraphicsPipeline 
{
public:
    struct RenderingResource
    {
        VkSemaphore       imageAvailableSema = VK_NULL_HANDLE;
        VkSemaphore       renderFinishedSema = VK_NULL_HANDLE;
        VkFence           inFlightFence      = VK_NULL_HANDLE;
        VkCommandBuffer*  commandBuffer       = nullptr;
    };

    MKGraphicsPipeline(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef);
	~MKGraphicsPipeline();

    /* lazy build of pipeline */
    void BuildPipeline(VkDescriptorSetLayout descriptorSetLayout, std::vector<VkDescriptorSet> descriptorSets, std::vector<VkPushConstantRange> pushConstants);

    /* getters */
    RenderingResource& GetRenderingResource(uint32 index) { return _renderingResources[index]; }
    VkPipelineLayout   GetPipelineLayout() const { return _vkPipelineLayout; }
    VkPipeline         GetPipeline() const { return _vkGraphicsPipeline; }

private:
    /* create rendering resources */
    void            CreateRenderingResources();

private:
    /* pipeline instance */
	VkPipeline	      _vkGraphicsPipeline;
	VkPipelineLayout  _vkPipelineLayout;

    /* rendering resources */
    std::vector<RenderingResource> _renderingResources;

    /* push constant */
    VkPushConstantRaster _vkPushConstantRaster{
        glm::mat4(1.0),
        glm::vec3(10.0f, 15.0f, 8.0f),
        0,
        100.0f,
        0
    };

private:
	MKDevice&     _mkDeviceRef;
    MKSwapchain&  _mkSwapchainRef;
};