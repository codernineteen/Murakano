#include "GraphicsPipeline.h" 

/*
-----------	PUBLIC ------------
*/
MKGraphicsPipeline::MKGraphicsPipeline(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef)
	:
	_mkDeviceRef(mkDeviceRef),
	_mkSwapchainRef(mkSwapchainRef)
{
	CreateRenderingResources();
}

MKGraphicsPipeline::~MKGraphicsPipeline()
{
	// destroy sync objects
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _renderingResources[i].renderFinishedSema, nullptr);
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _renderingResources[i].imageAvailableSema, nullptr);
		vkDestroyFence(_mkDeviceRef.GetDevice(), _renderingResources[i].inFlightFence, nullptr);
	}

	// destroy pipeline and pipeline layout
	vkDestroyPipeline(_mkDeviceRef.GetDevice(), _vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(_mkDeviceRef.GetDevice(), _vkPipelineLayout, nullptr);

#ifndef NDEBUG
	MK_LOG("sync objects destroyed");
	MK_LOG("graphics pipeline and its layout destroyed");
#endif
}

void MKGraphicsPipeline::BuildPipeline(
	VkDescriptorSetLayout& descriptorSetLayout, 
	std::vector<VkDescriptorSet>& descriptorSets, 
	std::vector<VkPushConstantRange>& pushConstants, 
	VkRenderPass& renderPass
)
{
#ifdef USE_HLSL
	// HLSL shader codes
	auto vertShaderCode = util::ReadFile("../../../shaders/output/spir-v/vertex.spv");
	auto fragShaderCode = util::ReadFile("../../../shaders/output/spir-v/fragment.spv");
#else
	// GLSL shader codes
	auto vertShaderCode = util::ReadFile("../../../shaders/output/spir-v/vertexShader.spv");
	auto fragShaderCode = util::ReadFile("../../../shaders/output/spir-v/fragmentShader.spv");
#endif

	VkShaderModule vertShaderModule = util::CreateShaderModule(_mkDeviceRef.GetDevice(), vertShaderCode); // create shader module   
	VkShaderModule fragShaderModule = util::CreateShaderModule(_mkDeviceRef.GetDevice(), fragShaderCode); // create shader module

	std::string shaderEntryPoint = "main";
	// vertex shader stage specification
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinfo::GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, shaderEntryPoint);
	// fragment shader stage specification
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinfo::GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, shaderEntryPoint);
	// shader stages
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex description
	auto bindingDescription     = Vertex::GetBindingDescription();
	auto attributeDescriptions  = Vertex::GetAttributeDescriptions();
	constexpr int attributeSize = attributeDescriptions.size();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinfo::GetPipelineVertexInputStateCreateInfo<attributeSize>(bindingDescription, attributeDescriptions);

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinfo::GetPipelineInputAssemblyStateCreateInfo();

	/**
	* viewport and scissor rectangle
	* - viewport : transformation from image to framebuffer
	* - scissor  : in which regions pixels will be stored
	*/
	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineViewportStateCreateInfo viewportState = vkinfo::GetPipelineViewportStateCreateInfo();
	VkPipelineDynamicStateCreateInfo dynamicState = vkinfo::GetPipelineDynamicStateCreateInfo(dynamicStates);

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = vkinfo::GetPipelineRasterizationStateCreateInfo();

	// TODO : multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = vkinfo::GetPipelineMultisampleStateCreateInfo();

	// depth and stencil testing (TODO : implement stencil buffer operation for shadow volume)
	VkPipelineDepthStencilStateCreateInfo depthStencil = vkinfo::GetPipelineDepthStencilStateCreateInfo();

	// TODO : color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinfo::GetPipelineColorBlendAttachmentState();

	// TODO : color blending state
	VkPipelineColorBlendStateCreateInfo colorBlending = vkinfo::GetPipelineColorBlendStateCreateInfo(colorBlendAttachment);

	/* create pipeline layout */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinfo::GetPipelineLayoutCreateInfo(&descriptorSetLayout, 1, pushConstants.size() > 0 ? pushConstants.data() : nullptr, (uint32)pushConstants.size());
	MK_CHECK(vkCreatePipelineLayout(_mkDeviceRef.GetDevice(), &pipelineLayoutInfo, nullptr, &_vkPipelineLayout));

	// specify graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = vkinfo::GetGraphicsPipelineCreateInfo(
		_vkPipelineLayout,
		shaderStages,
		&vertexInputInfo,
		&inputAssembly,
		&viewportState,
		&rasterizer,
		&multisampling,
		&depthStencil,
		&colorBlending,
		&dynamicState,
		_vkPipelineLayout,
		renderPass
	);

	MK_CHECK(vkCreateGraphicsPipelines(_mkDeviceRef.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_vkGraphicsPipeline));

	// destroy shader modules after creating a pipeline.
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), vertShaderModule, nullptr);
}

/*
-----------	PRIVATE ------------
*/

void MKGraphicsPipeline::CreateRenderingResources()
{
	_renderingResources.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // create fence as signaled state for the very first frame.

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		MK_CHECK(vkCreateSemaphore(_mkDeviceRef.GetDevice(), &semaphoreInfo, nullptr, &_renderingResources[i].imageAvailableSema));
		MK_CHECK(vkCreateSemaphore(_mkDeviceRef.GetDevice(), &semaphoreInfo, nullptr, &_renderingResources[i].renderFinishedSema));
		MK_CHECK(vkCreateFence(_mkDeviceRef.GetDevice(), &fenceInfo, nullptr, &_renderingResources[i].inFlightFence));
		_renderingResources[i].commandBuffer = GCommandService->GetCommandBuffer(i);
	}
}