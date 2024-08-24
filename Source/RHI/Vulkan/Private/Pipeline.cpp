#include "Pipeline.h" 

/*
-----------	PUBLIC ------------
*/
MKPipeline::MKPipeline(MKDevice& mkDeviceRef, std::string name)
	:
	_mkDeviceRef(mkDeviceRef), 
	_pipelineName(name)
{
}

MKPipeline::~MKPipeline()
{
	// destroy pipeline and pipeline layout
	vkDestroyPipeline(_mkDeviceRef.GetDevice(), _vkPipelineInstance, nullptr);
	vkDestroyPipelineLayout(_mkDeviceRef.GetDevice(), _vkPipelineLayout, nullptr);

#ifndef NDEBUG
	MK_LOG(_pipelineName + " pipeline destroyed.");
#endif
}

void MKPipeline::AddShader(const char* path, std::string entryPoint, VkShaderStageFlagBits stageBit)
{
	auto shaderCode = mk::file::ReadFile(path);

	VkShaderModule shaderModule = mk::vk::CreateShaderModule(_mkDeviceRef.GetDevice(), shaderCode); // create shader module, should be destroyed after creating a pipeline.

	ShaderDesc shaderDesc;
	shaderDesc.shaderModule = shaderModule;
	shaderDesc.entryPoint = entryPoint;
	shaderDesc.stage = stageBit;

	_waitShaders.push_back(shaderDesc);
}

void MKPipeline::SetDefaultPipelineCreateInfo()
{
	for (auto& shader : _waitShaders)
	{
		VkPipelineShaderStageCreateInfo shaderStageInfo = mk::vkinfo::GetPipelineShaderStageCreateInfo(shader.stage, shader.shaderModule, shader.entryPoint);
		shaderStages.push_back(shaderStageInfo);
	}

	// vertex description
	vertexInput = mk::vkinfo::GetPipelineVertexInputStateCreateInfo<3>(bindingDesc, attribDesc);

	// input assembly
	inputAssembly = mk::vkinfo::GetPipelineInputAssemblyStateCreateInfo();

	/**
	* viewport and scissor rectangle
	* - viewport : transformation from image to framebuffer
	* - scissor  : in which regions pixels will be stored
	*/
	viewportState = mk::vkinfo::GetPipelineViewportStateCreateInfo();
	dynamicState = mk::vkinfo::GetPipelineDynamicStateCreateInfo(dynamicStates);

	// rasterizer
	rasterizer = mk::vkinfo::GetPipelineRasterizationStateCreateInfo();

	// TODO : multisampling
	multisampling = mk::vkinfo::GetPipelineMultisampleStateCreateInfo();

	// depth and stencil testing (TODO : implement stencil buffer operation for shadow volume)
	depthStencil = mk::vkinfo::GetPipelineDepthStencilStateCreateInfo();

	// TODO : color blending
	colorBlendAttachment = mk::vkinfo::GetPipelineColorBlendAttachmentState();

	// TODO : color blending state
	colorBlending = mk::vkinfo::GetPipelineColorBlendStateCreateInfo(colorBlendAttachment);

	// create pipeline layout
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;
}

void MKPipeline::CreatePipelineLayout()
{
	// create pipeline layout
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;

	// create pipeline layout
	MK_CHECK(vkCreatePipelineLayout(_mkDeviceRef.GetDevice(), &pipelineLayoutInfo, nullptr, &_vkPipelineLayout));
}

void MKPipeline::BuildPipeline(VkRenderPass* pRenderPass)
{
	assert(_vkPipelineLayout != VK_NULL_HANDLE && "Pipeline layout is not created yet. Call CreatePipelineLayout() before building a pipeline.");

	// specify graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = mk::vkinfo::GetGraphicsPipelineCreateInfo(
		shaderStages,
		&vertexInput,
		&inputAssembly,
		&viewportState,
		&rasterizer,
		&multisampling,
		&depthStencil,
		&colorBlending,
		&dynamicState,
		&_vkPipelineLayout,
		pRenderPass,
		&renderingInfo
	);

	// create pipeline instance
	MK_CHECK(vkCreateGraphicsPipelines(_mkDeviceRef.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_vkPipelineInstance));

	// destroy shader modules after creating a pipeline.
	for (auto& shader : _waitShaders)
	{
		vkDestroyShaderModule(_mkDeviceRef.GetDevice(), shader.shaderModule, nullptr);
		shader.shaderModule = VK_NULL_HANDLE;
	}
	_waitShaders.clear();
}

void MKPipeline::AddDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& layouts)
{
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();
}

void MKPipeline::AddPushConstantRanges(std::vector<VkPushConstantRange>& pushConstants)
{
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32>(pushConstants.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
}

void MKPipeline::SetRenderingInfo(
	uint32 colorAttachmentCount, 
	VkFormat* pColorAttachmentFormats, 
	VkFormat depthAttachmentFormat, 
	VkFormat stencilAttachmentFormat
)
{
	renderingInfo.colorAttachmentCount = colorAttachmentCount;
	renderingInfo.pColorAttachmentFormats = pColorAttachmentFormats;
	renderingInfo.depthAttachmentFormat = depthAttachmentFormat;
	renderingInfo.stencilAttachmentFormat = stencilAttachmentFormat;
	renderingInfo.pNext = nullptr;
}

void MKPipeline::CopyPipelineLayoutCreateInfo(const VkPipelineLayoutCreateInfo& layout)
{
	pipelineLayoutInfo.sType = layout.sType;
	pipelineLayoutInfo.flags = layout.flags;
	pipelineLayoutInfo.setLayoutCount = layout.setLayoutCount;
	pipelineLayoutInfo.pSetLayouts = layout.pSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = layout.pushConstantRangeCount;
	pipelineLayoutInfo.pPushConstantRanges = layout.pPushConstantRanges;
}

void MKPipeline::SetInputTopology(VkPrimitiveTopology topology)
{
	inputAssembly.topology = topology;
}

void MKPipeline::SetPolygonMode(VkPolygonMode mode)
{
	rasterizer.polygonMode = mode;
}

void MKPipeline::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
	rasterizer.cullMode  = cullMode;
	rasterizer.frontFace = frontFace;
}

void MKPipeline::DisableMultiSampling()
{
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading      = 1.0f;
	multisampling.pSampleMask           = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable      = VK_FALSE;
}

void MKPipeline::DisableColorBlending()
{
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
}

void MKPipeline::EnableBlendingAdditive()
{
	colorBlendAttachment.blendEnable         = VK_TRUE;
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

void MKPipeline::EnableBlendingAlpha()
{
	colorBlendAttachment.blendEnable         = VK_TRUE;
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

void MKPipeline::DisableDepthTest()
{
	depthStencil.depthTestEnable       = VK_FALSE;
	depthStencil.depthWriteEnable      = VK_FALSE;
	depthStencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};
	depthStencil.back                  = {};
	depthStencil.minDepthBounds        = 0.f;
	depthStencil.maxDepthBounds        = 1.f;
}

void MKPipeline::EnableDepthTest(bool depthWriteEnable, VkCompareOp op)
{
	depthStencil.depthTestEnable       = VK_TRUE;
	depthStencil.depthWriteEnable      = depthWriteEnable;
	depthStencil.depthCompareOp        = op;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};
	depthStencil.back                  = {};
	depthStencil.minDepthBounds        = 0.f;
	depthStencil.maxDepthBounds        = 1.f;
}

void MKPipeline::RemoveVertexInput()
{
	vertexInput = {}; // assign empty struct
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}
