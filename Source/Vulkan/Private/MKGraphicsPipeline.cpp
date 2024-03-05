#include "MkGraphicsPipeline.h" 

/*
-----------	PUBLIC ------------
*/
MKGraphicsPipeline::MKGraphicsPipeline(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef)
	: 
	_mkDeviceRef(mkDeviceRef), 
	_mkSwapchainRef(mkSwapchainRef),
	_vikingRoom(OBJModel("../../../resources/Models/viking_room.obj", "../../../resources/Textures/viking_room.png"))
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


	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode); // create shader module   
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode); // create shader module

	std::string shaderEntryPoint = "main";
	// vertex shader stage specification
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinfo::GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, shaderEntryPoint);
	// fragment shader stage specification
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinfo::GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, shaderEntryPoint);
	// shader stages
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// vertex description
	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();
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
	VkPipelineRasterizationStateCreateInfo rasterizer  = vkinfo::GetPipelineRasterizationStateCreateInfo();

	// TODO : multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = vkinfo::GetPipelineMultisampleStateCreateInfo();

	// depth and stencil testing (TODO : implement stencil buffer operation for shadow volume)
	VkPipelineDepthStencilStateCreateInfo depthStencil = vkinfo::GetPipelineDepthStencilStateCreateInfo();

	// TODO : color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinfo::GetPipelineColorBlendAttachmentState();

	// TODO : color blending state
	VkPipelineColorBlendStateCreateInfo colorBlending = vkinfo::GetPipelineColorBlendStateCreateInfo(colorBlendAttachment);

	// specify pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinfo::GetPipelineLayoutCreateInfo(GDescriptorManager->GetDescriptorSetLayoutPtr());

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
		_mkSwapchainRef.RequestRenderPass()
	);

	MK_CHECK(vkCreateGraphicsPipelines(_mkDeviceRef.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_vkGraphicsPipeline));

	// create sync objects
	CreateSyncObjects();

	// create vertex buffer
	CreateVertexBuffer();

	// create index buffer
	CreateIndexBuffer();

	// destroy shader modules after creating a pipeline.
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), vertShaderModule, nullptr);
}

MKGraphicsPipeline::~MKGraphicsPipeline()
{
	// destroy buffers
	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), _vkVertexBuffer.buffer, _vkVertexBuffer.allocation);
	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), _vkIndexBuffer.buffer, _vkIndexBuffer.allocation);

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

#ifndef NDEBUG
	MK_LOG("index buffer destroyed and memory freed");
	MK_LOG("vertex buffer destroyed and memory freed");
	MK_LOG("sync objects destroyed");
	MK_LOG("graphics pipeline and its layout destroyed");
#endif
}

void MKGraphicsPipeline::RecordFrameBuffferCommand(uint32 swapchainImageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;					// Optional
	beginInfo.pInheritanceInfo = nullptr;	// Optional

	auto commandBuffer = GCommandService->GetCommandBuffer(_currentFrame);
	MK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

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
	clearValues[0] = { {0.0f, 0.0f, 0.0f, 1.0f} }; // clear values for color
	clearValues[1] = { 1.0f, 0 };                    // clear value for depth and stencil attachment
	renderPassInfo.clearValueCount = SafeStaticCast<size_t, uint32>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	// begin render pass
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);		// start render pass
	
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

	// Index of vertexBuffers and offsets should be the same as the number of binding points in the vertex shader
	VkBuffer vertexBuffers[] = { _vkVertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);					// bind vertex buffer
	vkCmdBindIndexBuffer(commandBuffer, _vkIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);           // bind index buffer
	
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		_vkPipelineLayout, 
		0, 
		1, 
		GDescriptorManager->GetDescriptorSetPtr(_currentFrame),
		0, 
		nullptr
	);
	vkCmdDrawIndexed(commandBuffer, _vikingRoom.indices.size(), 1, 0, 0, 0);

	// end render pass
	vkCmdEndRenderPass(commandBuffer);

	MK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void MKGraphicsPipeline::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(_vikingRoom.vertices[0]) * _vikingRoom.vertices.size();
	/**
	* Staging buffer : temporary host-visible buffer
	* - usage : source of memory transfer operation
	* - property : host-visible, host-coherent
	* - allocation flag : VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT must be set because of VMA_MEMORY_USAGE_AUTO , CREATE_MAPPED_BIT is set by default
	*/
	VkBufferAllocated stagingBuffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_AUTO, // auto-detect memory type
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
	);

	memcpy(stagingBuffer.allocationInfo.pMappedData, _vikingRoom.vertices.data(), (size_t)bufferSize);

	/**
	* Device local buffer : actual vertex buffer
	* - usage : destination of memory transfer operation, vertex buffer
	* - property : device local
	* Note that we can't map memory for device-local buffer, but can copy data to it.
	*/
	_vkVertexBuffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT
	);
	CopyBufferToBuffer(stagingBuffer, _vkVertexBuffer, bufferSize); // copy staging buffer to vertex buffer

	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}

void MKGraphicsPipeline::CreateIndexBuffer() 
{
	VkDeviceSize bufferSize = sizeof(_vikingRoom.indices[0]) * _vikingRoom.indices.size();
	VkBufferAllocated stagingBuffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VMA_MEMORY_USAGE_AUTO, // auto-detect memory type
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT// must to be set because of VMA_MEMORY_USAGE_AUTO
	);

	memcpy(stagingBuffer.allocationInfo.pMappedData, _vikingRoom.indices.data(), (size_t)bufferSize);
	/**
	* Device local buffer : actual index buffer
	* - usage : destination of memory transfer operation, index buffer
	* - property : device local
	*/
	_vkIndexBuffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT
	);

	CopyBufferToBuffer(stagingBuffer, _vkIndexBuffer, bufferSize);

	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}

/*
-----------	PRIVATE ------------
*/

VkShaderModule MKGraphicsPipeline::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32*>(code.data()); // guaranteed to be aligned by default allocator  of std::vector 

	VkShaderModule shaderModule;
	MK_CHECK(vkCreateShaderModule(_mkDeviceRef.GetDevice(), &createInfo, nullptr, &shaderModule));
	
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

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		MK_CHECK(vkCreateSemaphore(_mkDeviceRef.GetDevice(), &semaphoreInfo, nullptr, &_vkImageAvailableSemaphores[i]));
		MK_CHECK(vkCreateSemaphore(_mkDeviceRef.GetDevice(), &semaphoreInfo, nullptr, &_vkRenderFinishedSemaphores[i]));
		MK_CHECK(vkCreateFence(_mkDeviceRef.GetDevice(), &fenceInfo, nullptr, &_vkInFlightFences[i]));
	}
}


void MKGraphicsPipeline::DrawFrame()
{
	auto device = _mkDeviceRef.GetDevice();
	auto swapChain = _mkSwapchainRef.GetSwapchain();

	// 1. wait until the previous frame is finished
	vkWaitForFences(device, 1, &_vkInFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

	// 2. get an image from swap chain
	uint32 imageIndex; // index of the swap chain image that has become available. filled by vkAcquireNextImageKHR
	// semaphore to wait presentation engine to finish presentation here.
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, _vkImageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		_mkSwapchainRef.RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	GDescriptorManager->UpdateUniformBuffer(_currentFrame);

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
		MK_THROW("failed to present swap chain image!");

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // circular update of current frame.
}

void MKGraphicsPipeline::CopyBufferToBuffer(VkBufferAllocated src, VkBufferAllocated dest, VkDeviceSize size)
{
	// a command to copy buffer to buffer.
	GCommandService->ExecuteSingleTimeCommands([&](VkCommandBuffer commandBuffer) { // getting reference parameter outer-scope
		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, src.buffer, dest.buffer, 1, &copyRegion);
	});
}
