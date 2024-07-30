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

class MKPipeline 
{
public:
    struct ShaderDesc
    {
        VkShaderModule        shaderModule;
        VkShaderStageFlagBits stage;
        std::string           entryPoint;
    };

    struct RenderingResource
    {
        VkSemaphore       imageAvailableSema = VK_NULL_HANDLE;
        VkSemaphore       renderFinishedSema = VK_NULL_HANDLE;
        VkFence           inFlightFence      = VK_NULL_HANDLE;
        VkCommandBuffer*  commandBuffer       = nullptr;
    };

    MKPipeline(MKDevice& mkDeviceRef);
	~MKPipeline();

    /* getters */
    RenderingResource& GetRenderingResource(uint32 index) { return _renderingResources[index]; }
    VkPipelineLayout   GetPipelineLayout() const { return _vkPipelineLayout; }
    VkPipeline         GetPipeline() const { return _vkGraphicsPipeline; }
    
    /* api */
    void AddShader(const char* path, std::string entryPoint, VkShaderStageFlagBits stageBit);
    void CreateDefaultPipelineLayout(
        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
        std::vector<VkPushConstantRange>& pushConstants
    );
    void BuildPipeline(VkRenderPass& renderPass);

private:
    /* create rendering resources */
    void            CreateRenderingResources();

private:
    /* pipeline states */
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    std::vector<VkDynamicState>                  _dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkVertexInputBindingDescription                 _bindingDesc = Vertex::GetBindingDescription();
    std::array<VkVertexInputAttributeDescription,3> _attribDesc = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo   _vertexInput;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineViewportStateCreateInfo      _viewportState;
    VkPipelineDynamicStateCreateInfo       _dynamicState;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineMultisampleStateCreateInfo   _multisampling;
    VkPipelineDepthStencilStateCreateInfo  _depthStencil;
    VkPipelineColorBlendAttachmentState    _colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo    _colorBlending;
    VkPipelineLayoutCreateInfo             _pipelineLayoutInfo;

    /* pipeline instance */
	VkPipeline	      _vkGraphicsPipeline;
	VkPipelineLayout  _vkPipelineLayout;

    /* rendering resources */
    std::vector<RenderingResource> _renderingResources;

    /* shader stages waiting initialization */
    std::vector<ShaderDesc> _waitShaders;

private:
	MKDevice&     _mkDeviceRef;
};