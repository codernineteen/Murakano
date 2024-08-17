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
    };

    MKPipeline(MKDevice& mkDeviceRef);
	~MKPipeline();

    /* getters */
    RenderingResource& GetRenderingResource(uint32 index) { return _renderingResources[index]; }
    VkPipelineLayout   GetPipelineLayout() const { return _vkPipelineLayout; }
    VkPipeline         GetPipeline() const { return _vkPipelineInstance; }
    
    /**
    * API 
    */
    void AddShader(const char* path, std::string entryPoint, VkShaderStageFlagBits stageBit);
    void AddDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& layouts);
    void AddPushConstantRanges(std::vector<VkPushConstantRange>& pushConstants);
    void InitializePipelineLayout();

    /* pipeline state modifier */
    void SetRenderingInfo(
        uint32 colorAttachmentCount,
        VkFormat* pColorAttachmentFormats,
        VkFormat depthAttachmentFormat,
        VkFormat stencilAttachmentFormat
    );
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void DisableMultiSampling();
    void DisableColorBlending();
    void EnableBlendingAdditive();
    void EnableBlendingAlpha();
    void DisableDepthTest();
    void EnableDepthTest(bool depthWriteEnable, VkCompareOp op);
    void RemoveVertexInput();

    /* finialize pipeline and compile */
    void BuildPipeline(VkRenderPass* pRenderPass = nullptr);

private:
    /* create rendering resources */
    void CreateRenderingResources();

public:
    /* pipeline states */
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkDynamicState>                  dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkVertexInputBindingDescription                 bindingDesc = Vertex::GetBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> attribDesc = Vertex::GetAttributeDescriptions();

    VkPipelineRenderingCreateInfoKHR       renderingInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    VkPipelineVertexInputStateCreateInfo   vertexInput;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineViewportStateCreateInfo      viewportState;
    VkPipelineDynamicStateCreateInfo       dynamicState;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo   multisampling;
    VkPipelineDepthStencilStateCreateInfo  depthStencil;
    VkPipelineColorBlendAttachmentState    colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo    colorBlending;
    VkPipelineLayoutCreateInfo             pipelineLayoutInfo{ // initialize with default values
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

private:
    /* pipeline instance */
	VkPipeline	      _vkPipelineInstance;
	VkPipelineLayout  _vkPipelineLayout;

    /* rendering resources */
    std::vector<RenderingResource> _renderingResources;

    /* shader stages waiting initialization */
    std::vector<ShaderDesc> _waitShaders;

private:
	MKDevice&     _mkDeviceRef;
};