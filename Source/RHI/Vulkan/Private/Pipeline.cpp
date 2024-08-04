#include "Pipeline.h" 

/*
-----------	PUBLIC ------------
*/
MKPipeline::MKPipeline(MKDevice& mkDeviceRef)
	:
	_mkDeviceRef(mkDeviceRef)
{
	CreateRenderingResources();
}

MKPipeline::~MKPipeline()
{
	// destroy sync objects
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _renderingResources[i].renderFinishedSema, nullptr);
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _renderingResources[i].imageAvailableSema, nullptr);
		vkDestroyFence(_mkDeviceRef.GetDevice(), _renderingResources[i].inFlightFence, nullptr);
	}

	// destroy pipeline and pipeline layout
	vkDestroyPipeline(_mkDeviceRef.GetDevice(), _vkPipelineInstance, nullptr);
	vkDestroyPipelineLayout(_mkDeviceRef.GetDevice(), _vkPipelineLayout, nullptr);

#ifndef NDEBUG
	MK_LOG("sync objects destroyed");
	MK_LOG("graphics pipeline and its layout destroyed");
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

void MKPipeline::CreateDefaultPipelineLayout(
	std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
	std::vector<VkPushConstantRange>& pushConstants
)
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
	pipelineLayoutInfo = mk::vkinfo::GetPipelineLayoutCreateInfo(
		descriptorSetLayouts.size() > 0 ? descriptorSetLayouts.data() : nullptr,
		static_cast<uint32>(descriptorSetLayouts.size()),
		pushConstants.size() > 0 ? pushConstants.data() : nullptr,
		static_cast<uint32>(pushConstants.size())
	);
	MK_CHECK(vkCreatePipelineLayout(_mkDeviceRef.GetDevice(), &pipelineLayoutInfo, nullptr, &_vkPipelineLayout));
}

void MKPipeline::BuildPipeline(VkRenderPass& renderPass)
{
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
		_vkPipelineLayout,
		renderPass
	);

	// create pipeline instance
	MK_CHECK(vkCreateGraphicsPipelines(_mkDeviceRef.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_vkPipelineInstance));

	// destroy shader modules after creating a pipeline.
	for (auto& shader : _waitShaders)
	{
		vkDestroyShaderModule(_mkDeviceRef.GetDevice(), shader.shaderModule, nullptr);
		shader.shaderModule = VK_NULL_HANDLE;
	}
}

/*
-----------	PRIVATE ------------
*/

void MKPipeline::CreateRenderingResources()
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