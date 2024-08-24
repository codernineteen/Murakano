#pragma once

// internal
#include "Utilities.h"
#include "Global.h"
#include "Vertex.h"

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

    MKPipeline(MKDevice& mkDeviceRef, std::string name);
	~MKPipeline();

    /* getters */
    //RenderingResource&         GetRenderingResource(uint32 index) { return _renderingResources[index]; }
    VkPipelineLayoutCreateInfo GetPipelineLayoutCreateInfo() const { return pipelineLayoutInfo; }
    VkPipelineLayout           GetPipelineLayout() const { return _vkPipelineLayout; }
    VkPipeline                 GetPipeline() const { return _vkPipelineInstance; }
    VkPipeline*                GetPipelinePtr() { return &_vkPipelineInstance; }
    
    /**
    * API 
    */
    void AddShader(const char* path, std::string entryPoint, VkShaderStageFlagBits stageBit);
    void AddDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& layouts);
    void AddPushConstantRanges(std::vector<VkPushConstantRange>& pushConstants);

    /* pipeline state modifier */
    void CopyPipelineLayoutCreateInfo(const VkPipelineLayoutCreateInfo& layout);
    void CopyPipelineLayout(const VkPipelineLayout& pipelineLayout) { _vkPipelineLayout = pipelineLayout; };
    void SetDefaultPipelineCreateInfo();
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
    void CreatePipelineLayout();
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
	VkPipeline	      _vkPipelineInstance{ VK_NULL_HANDLE };
    VkPipelineLayout  _vkPipelineLayout{ VK_NULL_HANDLE };
    std::string       _pipelineName;

    /* shader stages waiting initialization */
    std::vector<ShaderDesc> _waitShaders;

private:
	MKDevice&     _mkDeviceRef;
};