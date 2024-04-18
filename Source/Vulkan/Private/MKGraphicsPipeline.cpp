#include "MkGraphicsPipeline.h" 

/*
-----------	PUBLIC ------------
*/
MKGraphicsPipeline::MKGraphicsPipeline(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef)
	: 
	_mkDeviceRef(mkDeviceRef), 
	_mkSwapchainRef(mkSwapchainRef),
	vikingRoom(OBJModel(mkDeviceRef, "../../../resources/Models/viking_room.obj", "../../../resources/Textures/viking_room.png")),
	_camera(mkDeviceRef, mkSwapchainRef),
	_inputController(mkDeviceRef.GetWindowRef().GetWindow(), _camera)
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
	// create rendering resources
	CreateRenderingResources();

	// create vertex buffer
	CreateVertexBuffer();

	// create index buffer
	CreateIndexBuffer();

	// create uniform buffer
	CreateUniformBuffers();

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

	/**
	* add descriptor set layout binding
	* 1. descriptor type
	* 2. shader stage flags
	* 3. binding point
	* 4. descriptor count
	*/

	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                           
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		0,								                             
		1								                            
	);
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		1,
		1
	);

	// create descriptor set layout with bindings
	GDescriptorManager->CreateDescriptorSetLayout(_vkDescriptorSetLayout);
	// allocate descriptor set layout
	GDescriptorManager->AllocateDescriptorSet(_vkDescriptorSets, _vkDescriptorSetLayout);

	// update descriptor set
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		GDescriptorManager->WriteBufferToDescriptorSet(
			_vkUniformBuffers[i].buffer,               // uniform buffer
			0,                                         // offset
			sizeof(UniformBufferObject),               // range
			0,                                         // binding point
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER          // descriptor type
		);
		// texture image view and sampler is used commonly in descriptor sets
		GDescriptorManager->WriteImageToDescriptorSet(
			vikingRoom.vikingTexture.imageView,        // texture image view
			vikingRoom.vikingTexture.sampler,          // texture sampler
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // image layout
			1,                                         // binding point
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER  // descriptor type
		);
		// update descriptor set
		GDescriptorManager->UpdateDescriptorSet(_vkDescriptorSets[i]);
	}
	VkPushConstantRange pushConstantRanges{};
	pushConstantRanges.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRanges.offset = 0;
	pushConstantRanges.size = sizeof(VkPushConstantRaster);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinfo::GetPipelineLayoutCreateInfo(&_vkDescriptorSetLayout, &pushConstantRanges);

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

	// destroy shader modules after creating a pipeline.
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(_mkDeviceRef.GetDevice(), vertShaderModule, nullptr);

	// build ray tracer
	auto vertexAddr = mkDeviceRef.GetBufferDeviceAddress(_vkVertexBuffer.buffer);
	auto indexAddr = mkDeviceRef.GetBufferDeviceAddress(_vkIndexBuffer.buffer);
}

MKGraphicsPipeline::~MKGraphicsPipeline()
{
	// destroy storage image
	util::DestroyImageResource(_mkDeviceRef.GetVmaAllocator(), _mkDeviceRef.GetDevice(), _vkStorageImage.imageAllocated, _vkStorageImage.imageView);


	// destroy buffers
	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), _vkVertexBuffer.buffer, _vkVertexBuffer.allocation);
	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), _vkIndexBuffer.buffer, _vkIndexBuffer.allocation);
	for (auto& uniformBuffer : _vkUniformBuffers)
		vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), uniformBuffer.buffer, uniformBuffer.allocation);


	// destroy descriptor set layout
	vkDestroyDescriptorSetLayout(_mkDeviceRef.GetDevice(), _vkDescriptorSetLayout, nullptr);

	// destroy sync objects
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _renderingResources[i].renderFinishedSema, nullptr);
		vkDestroySemaphore(_mkDeviceRef.GetDevice(), _renderingResources[i].imageAvailableSema, nullptr);
		vkDestroyFence(_mkDeviceRef.GetDevice(), _renderingResources[i].inFlightFence, nullptr);
	}

	// destroy ray tracer resources
	delete GRaytracer;

	// destroy pipeline and pipeline layout
	vkDestroyPipeline(_mkDeviceRef.GetDevice(), _vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(_mkDeviceRef.GetDevice(), _vkPipelineLayout, nullptr);

#ifndef NDEBUG
	MK_LOG("storage image destroyed and memorr freed");
	MK_LOG("index buffer destroyed and memory freed");
	MK_LOG("vertex buffer destroyed and memory freed");
	MK_LOG("uniform buffers destroyed and memory freed");
	MK_LOG("descriptor set layout destroyed");
	MK_LOG("sync objects destroyed");
	MK_LOG("graphics pipeline and its layout destroyed");
	MK_LOG("Free Global raytracer instance in graphics pipeline");
#endif
}

void MKGraphicsPipeline::RecordFrameBufferCommand(uint32 swapchainImageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;					// Optional
	beginInfo.pInheritanceInfo = nullptr;	// Optional

	auto commandBuffer = *(_renderingResources[_currentFrame].commandBuffer);
	MK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	/**
	* Copy ray tracing output to swapchain image
	*/
	// 1. prepare current swapchain image for transfer destination
	

	/**
	* Prepare render pass !
	*/
	// store swapchain extent for common usage.
	auto swapchainExtent = _mkSwapchainRef.GetSwapchainExtent();

	// settings for VK_ATTACHMENT_LOAD_OP_CLEAR in color attachment
	const auto clearColor = glm::vec4(1, 1, 1, 1.00f);
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0] = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};   // clear values for color
	clearValues[1] = { 1.0f, 0 };                    // clear value for depth and stencil attachment

	// specify render pass information
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _mkSwapchainRef.RequestRenderPass();
	renderPassInfo.framebuffer = _mkSwapchainRef.GetFramebuffer(swapchainImageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainExtent;
	renderPassInfo.clearValueCount = static_cast<uint32>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	// begin render pass
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);		// start render pass
	
	// set viewport and scissor for drawing
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

	// bind graphics pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vkGraphicsPipeline); // bind graphics pipeline
	// bind descriptor sets
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		_vkPipelineLayout, 
		0, 
		1, 
		_vkDescriptorSets.data(), // number of descriptor sets should fit into MAX_FRAMES_IN_FLIGHT
		0, 
		nullptr
	);
	// bind push constants
	vkCmdPushConstants(
		commandBuffer,
		_vkPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(VkPushConstantRaster),
		&_vkPushConstantRaster
	);

	// Index of vertexBuffers and offsets should be the same as the number of binding points in the vertex shader
	VkBuffer vertexBuffers[] = { _vkVertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);					// bind vertex buffer
	vkCmdBindIndexBuffer(commandBuffer, _vkIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);    // bind index buffer
	vkCmdDrawIndexed(commandBuffer, vikingRoom.indices.size(), 1, 0, 0, 0);

	// end render pass
	vkCmdEndRenderPass(commandBuffer);

	MK_CHECK(vkEndCommandBuffer(commandBuffer));
}


void MKGraphicsPipeline::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vikingRoom.vertices[0]) * vikingRoom.vertices.size();
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

	memcpy(stagingBuffer.allocationInfo.pMappedData, vikingRoom.vertices.data(), (size_t)bufferSize);

	/**
	* Device local buffer : actual vertex buffer
	* - usage : destination of memory transfer operation, vertex buffer
	* - property : device local
	* Note that we can't map memory for device-local buffer, but can copy data to it.
	*/
	_vkVertexBuffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | vkRayTracingFlags,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT
	);
	CopyBufferToBuffer(stagingBuffer, _vkVertexBuffer, bufferSize); // copy staging buffer to vertex buffer
	
	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}

void MKGraphicsPipeline::CreateIndexBuffer() 
{
	VkDeviceSize bufferSize = sizeof(vikingRoom.indices[0]) * vikingRoom.indices.size();
	VkBufferAllocated stagingBuffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VMA_MEMORY_USAGE_AUTO, // auto-detect memory type
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT// must to be set because of VMA_MEMORY_USAGE_AUTO
	);

	memcpy(stagingBuffer.allocationInfo.pMappedData, vikingRoom.indices.data(), (size_t)bufferSize);
	/**
	* Device local buffer : actual index buffer
	* - usage : destination of memory transfer operation, index buffer
	* - property : device local
	*/
	_vkIndexBuffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | vkRayTracingFlags,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT
	);

	CopyBufferToBuffer(stagingBuffer, _vkIndexBuffer, bufferSize);

	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}

void MKGraphicsPipeline::CreateUniformBuffers()
{
	// create uniform buffer object
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	_vkUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	_vkUniformBuffersMappedData.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		_vkUniformBuffers[i] = util::CreateBuffer(
			_mkDeviceRef.GetVmaAllocator(),
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			VMA_ALLOCATION_CREATE_MAPPED_BIT
		);

		_vkUniformBuffersMappedData[i] = _vkUniformBuffers[i].allocationInfo.pMappedData;
	}
}

void MKGraphicsPipeline::UpdateUniformBuffer(float time)
{
	UniformBufferObject ubo{};
	_camera.UpdateViewTarget();
#ifdef USE_HLSL
	auto projViewMat = _camera.GetProjectionMatrix() * _camera.GetViewMatrix();

	// initialize model transformation
	auto modelMat = XMMatrixIdentity();

	// Because SIMD operation is supported, i did multiplication in application side, not in shader side.
	ubo.mvpMat = projViewMat * modelMat;
#else
	// fill out uniform buffer object members
	ubo.modelMat = glm::mat4(1.0f);
	ubo.viewMat = _camera.GetViewMatrix();
	ubo.projMat = _camera.GetProjectionMatrix();
#endif

	memcpy(_vkUniformBuffersMappedData[_currentFrame], &ubo, sizeof(ubo));
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


void MKGraphicsPipeline::DrawFrame()
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	auto elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	// update delta time
	_deltaTime = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - _lastTime).count();
	_deltaTime /= 1000.0f;
	_lastTime = currentTime;

	// device references
	auto device = _mkDeviceRef.GetDevice();
	auto swapChain = _mkSwapchainRef.GetSwapchain();

	// 1. wait until the previous frame is finished
	vkWaitForFences(device, 1, &_renderingResources[_currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

	// 2. get an image from swap chain
	uint32 imageIndex; // index of the swap chain image that has become available. filled by vkAcquireNextImageKHR
	// semaphore to wait presentation engine to finish presentation here.
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, _renderingResources[_currentFrame].imageAvailableSema, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		_mkSwapchainRef.RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// update movement of camera
	_inputController.moveInPlaneXY(_deltaTime);
	// update rotation of camera
	_inputController.rotateCamera(_deltaTime);

	// update uniform buffer state
	UpdateUniformBuffer(elapsedTime);

	// To avoid deadlock on wait fence, only reset the fence if we are submmitting work
	vkResetFences(device, 1, &_renderingResources[_currentFrame].inFlightFence); // reset fence to unsignaled state manually

	// 2. reset command buffer , start recording commands for drawing.
	GCommandService->ResetCommandBuffer(_currentFrame);
	RecordFrameBufferCommand(imageIndex);

	// sync objects for command buffer submission
	VkSemaphore waitSemaphores[] = { _renderingResources[_currentFrame].imageAvailableSema };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { _renderingResources[_currentFrame].renderFinishedSema }; // a semaphore to signal when the command buffer has finished execution

	GCommandService->SubmitCommandBufferToQueue(
		_currentFrame,
		waitSemaphores, 
		waitStages,
		signalSemaphores, 
		_mkDeviceRef.GetGraphicsQueue(), 
		_renderingResources[_currentFrame].inFlightFence
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
		_mkSwapchainRef.RequestFramebufferResize(false); // framebuffer resize is not required
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
