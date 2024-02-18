#include "MkGraphicsPipeline.h" 

/*
-----------	PUBLIC ------------
*/
MKGraphicsPipeline::MKGraphicsPipeline(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef)
	: _mkDeviceRef(mkDeviceRef), _mkSwapchainRef(mkSwapchainRef)
{
	auto vertShaderCode = util::ReadFile("../../../shaders/vertexShader.spv");
	auto fragShaderCode = util::ReadFile("../../../shaders/fragmentShader.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode); // create shader module   
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode); // create shader module

	// vertex shader stage specification
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr; // compiler optimization purpose

	// fragment shader stage specification
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;


	// shader stages
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex description
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
	/*vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();*/

	// input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;	// keep the triangle list topology without restarting the assembly

	// viewport and scissor rectangle
	// viewport - defines transformation from image to framebuffer
	// scissor  - defines in which regions pixels will be stored
	VkPipelineViewportStateCreateInfo viewportState{};	// viewport and scissor rectangle
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;					// number of viewports
	viewportState.scissorCount = 1;						// number of scissor rectangles
	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;					// if true, fragments beyond near and far planes are clamped instead of discarded
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;							// thickness of lines in terms of number of fragments
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;			// back face culling setting
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // vertex order for faces to be considered front-facing
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;				// Optional
	rasterizer.depthBiasClamp = 0.0f;						// Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;					// Optional

	// TODO : multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// TODO : depth and stencil testing

	// TODO : color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;										  // RGBA channel
	colorBlendAttachment.blendEnable = VK_FALSE;						// False -  framebuffer unmodified
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				// Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	// Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				// Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;		// set true to combine color values using bitwise operation
	colorBlending.logicOp = VK_LOGIC_OP_COPY;	// Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;		// Optional
	colorBlending.blendConstants[1] = 0.0f;		// Optional
	colorBlending.blendConstants[2] = 0.0f;		// Optional
	colorBlending.blendConstants[3] = 0.0f;		// Optional

	// TODO : pipeline layout (need to specify descriptor)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;				// a descriptor set layout for uniform buffer object is set
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;		// Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr;	// Optional

	if (vkCreatePipelineLayout(_mkDeviceRef.GetDevice(), &pipelineLayoutInfo, nullptr, &_vkPipelineLayout) != VK_SUCCESS) 
		throw std::runtime_error("failed to create pipeline layout.");

	// specify graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;						// vertex and fragment shader
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	//pipelineInfo.pDepthStencilState = &depthStencil;	// Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = _vkPipelineLayout;			// pipeline layout
	pipelineInfo.renderPass = _mkSwapchainRef.RequestRenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// Optional - usage for piepline derivatives
	pipelineInfo.basePipelineIndex = -1;				// Optional

	if (vkCreateGraphicsPipelines(_mkDeviceRef.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_vkGraphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	// create sync objects
	CreateSyncObjects();

	// destroy shader modules after creating a pipeline.
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), vertShaderModule, nullptr);
}

MKGraphicsPipeline::~MKGraphicsPipeline()
{
	// destroy sync objects
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _vkRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _vkImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(_mkDeviceRef.GetDevice(), _vkInFlightFences[i], nullptr);
	}

	// destroy pipeline and pipeline layout
	vkDestroyPipeline(_mkDeviceRef.GetDevice(), _vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(_mkDeviceRef.GetDevice(), _vkPipelineLayout, nullptr);
}

void MKGraphicsPipeline::RecordFrameBuffferCommand(uint32_t swapchainImageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;					// Optional
	beginInfo.pInheritanceInfo = nullptr;	// Optional

	auto commandBuffer = GCommandService->GetCommandBuffer(_currentFrame);
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer!");

	// store swapchain extent for common usage.
	auto swapchainExtent = _mkSwapchainRef.GetSwapchainExtent();

	// specify render pass information
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _mkSwapchainRef.RequestRenderPass();
	renderPassInfo.framebuffer = _mkSwapchainRef.GetFramebuffer(swapchainImageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainExtent;

	// settings for VK_ATTACHMENT_LOAD_OP_CLEAR in color attachment
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0] = { {{0.0f, 0.0f, 0.0f, 1.0f}} }; // clear color
	clearValues[1] = { 1.0f, 0 }; // clear depth
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // start render pass
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vkGraphicsPipeline); // bind graphics pipeline

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer!");
}



/*
-----------	PRIVATE ------------
*/

VkShaderModule MKGraphicsPipeline::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // guaranteed to be aligned by default allocator  of std::vector 

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_mkDeviceRef.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
		throw std::runtime_error("failed to create shader module!");
	
	return shaderModule;
}

void MKGraphicsPipeline::CreateSyncObjects()
{
	_vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // create fence as signaled state for the very first frame.

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (
			vkCreateSemaphore(_mkDeviceRef.GetDevice(), &semaphoreInfo, nullptr, &_vkImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(_mkDeviceRef.GetDevice(), &semaphoreInfo, nullptr, &_vkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(_mkDeviceRef.GetDevice(), &fenceInfo, nullptr, &_vkInFlightFences[i]) != VK_SUCCESS
			)
			throw std::runtime_error("failed to create semaphores!");
	}
}

void MKGraphicsPipeline::DrawFrame()
{
	auto device = _mkDeviceRef.GetDevice();
	auto swapChain = _mkSwapchainRef.GetSwapchain();
	
	// 1. wait until the previous frame is finished
	vkWaitForFences(device, 1, &_vkInFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

	// 2. get an image from swap chain
	uint32_t imageIndex; // index of the swap chain image that has become available. filled by vkAcquireNextImageKHR
	// semaphore to wait presentation engine to finish presentation here.
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, _vkImageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		_mkSwapchainRef.RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	// updateUniformBuffer(currentFrame);

	// To avoid deadlock on wait fence, only reset the fence if we are submmitting work
	vkResetFences(device, 1, &_vkInFlightFences[_currentFrame]); // reset fence to unsignaled state manually

	// 2. reset command buffer , start recording commands for drawing.
	GCommandService->ResetCommandBuffer(_currentFrame);
	RecordFrameBuffferCommand(imageIndex);

	// sync objects for command buffer submission
	VkSemaphore waitSemaphores[] = { _vkImageAvailableSemaphores[_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { _vkRenderFinishedSemaphores[_currentFrame] }; // a semaphore to signal when the command buffer has finished execution

	GCommandService->SubmitCommandBufferToQueue(
		_currentFrame,
		waitSemaphores, 
		waitStages,
		signalSemaphores, 
		_mkDeviceRef.GetGraphicsQueue(), 
		_vkInFlightFences[_currentFrame]
	);

	// presentation
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores; // use same signal semaphore to wait on command buffer to finish execution

	// specify the swap chains to present images to and the index of the image for each swap chain
	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional - an array of VkResult to check for every individual swap chain if presentation was successful

	result = vkQueuePresentKHR(_mkDeviceRef.GetPresentQueue(), &presentInfo); // submit the request to present an image to the swap chain

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
	{
		// swapchain recreation is required at this moment.
		_mkSwapchainRef.RequestFramebufferResize(false);
		_mkSwapchainRef.RecreateSwapchain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("failed to present swap chain image!");

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // circular update of current frame.
}
