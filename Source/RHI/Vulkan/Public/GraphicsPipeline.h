#pragma once

// internal
#include "Utilities.h"
#include "Global.h"
#include "OBJModel.h"
#include "Texture.h"
#include "FreeCamera.h"
#include "InputController.h"

// RHI
#include "Device.h"
#include "Swapchain.h"
#include "DescriptorManager.h"

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
    void BuildPipeline(
        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
        std::vector<VkPushConstantRange>& pushConstants, 
        VkRenderPass& renderPass
    );

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

private:
	MKDevice&     _mkDeviceRef;
    MKSwapchain&  _mkSwapchainRef;
};