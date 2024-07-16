#include "Renderer.h"

Renderer::Renderer()
	: 
	_mkWindow(true), 
	_mkInstance(), 
	_mkDevice(_mkWindow, _mkInstance), 
	_mkSwapchain(_mkDevice),
	_mkGraphicsPipeline(_mkDevice, _mkSwapchain),
	_vikingModel(_mkDevice, "../../../resources/Models/viking_room.obj", "../../../resources/Textures/viking_room.png"),
	_camera(_mkDevice, _mkSwapchain),
	_inputController(_mkWindow.GetWindow(), _camera)
{
	GAllocator->InitVMAAllocator(_mkInstance.GetVkInstance(), _mkDevice.GetPhysicalDevice(), _mkDevice.GetDevice());
}

Renderer::~Renderer()
{
	// destroy buffers
	GAllocator->DestroyBuffer(_vkVertexBuffer);
	GAllocator->DestroyBuffer(_vkIndexBuffer);

	for(auto& uniformBuffer : _vkUniformBuffers)
		GAllocator->DestroyBuffer(uniformBuffer);

	// destroy descriptor set layout
	vkDestroyDescriptorSetLayout(_mkDevice.GetDevice(), _vkDescriptorSetLayout, nullptr);

	// destroy render pass
	vkDestroyRenderPass(_mkDevice.GetDevice(), _vkRenderPass, nullptr);

	// destroy allocator instance
	delete GAllocator;
#ifndef NDEBUG
	MK_LOG("descriptor set layout destroyed");
#endif
}

void Renderer::Setup()
{
	// query required color & depth attachment formats
	VkFormat swapchainImageFormat = _mkSwapchain.GetSwapchainImageFormat(); // color attachment format
	VkFormat depthFormat = util::FindDepthFormat(_mkDevice.GetPhysicalDevice()); // depth attachment format
	
	// create render pass
	mkvk::CreateDefaultRenderPass(_mkDevice.GetDevice(), swapchainImageFormat, depthFormat, &_vkRenderPass);

	// request creation of frame buffers
	_mkSwapchain.CreateFrameBuffers(_vkRenderPass);

	// create vertex buffer
	CreateVertexBuffer(_vikingModel.vertices);

	// create index buffer
	CreateIndexBuffer(_vikingModel.indices);

	// create push constant for rasterization
	CreatePushConstantRaster();

	// create uniform buffers
	CreateUniformBuffers();

	// append descriptor set layout binding
	AppendDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);
	AppendDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	// finalize descriptor set layout and allocate descriptor set for each frame
	CreateDescriptorSet();

	// update model descriptor set
	UpdateModelDescriptor();

	// build graphics pipeline
	_mkGraphicsPipeline.BuildPipeline(_vkDescriptorSetLayout, _vkDescriptorSets, _vkPushConstantRanges, _vkRenderPass);
}

void Renderer::CreateVertexBuffer(std::vector<Vertex> vertices)
{
	assert(vertices.size() > 0);

	uint64 numVertices = static_cast<uint64>(vertices.size());
	uint64 perVertexSize = static_cast<uint64>(sizeof(vertices[0]));
	VkDeviceSize bufferSize = numVertices * perVertexSize;

	/**
	* Staging buffer : temporary host-visible buffer
	* - usage : source of memory transfer operation
	* - property : host-visible, host-coherent
	* - allocation flag : VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT must be set because of VMA_MEMORY_USAGE_AUTO , CREATE_MAPPED_BIT is set by default
	*/
	VkBufferAllocated stagingBuffer;
	GAllocator->CreateBuffer(
		&stagingBuffer,
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VMA_MEMORY_USAGE_AUTO, // following VMA docs's best practice 
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		"vertex staging buffer"
	);

	// copy memory from vertices to staging buffer
	memcpy(stagingBuffer.allocationInfo.pMappedData, vertices.data(), (size_t)(bufferSize)); // c-style cast to avoid warning

	/**
	* Device local buffer : actual vertex buffer
	* - usage : destination of memory transfer operation, vertex buffer
	* - property : device local
	* Note that we can't map memory for device-local buffer, but can copy data to it.
	*/
	GAllocator->CreateBuffer(
		&_vkVertexBuffer,
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // plan to add ray tracing flags later
		VMA_MEMORY_USAGE_GPU_ONLY, 
		VMA_ALLOCATION_CREATE_MAPPED_BIT, 
		"vertex buffer"
	);

	// copy staging buffer to vertex buffer
	CopyBufferToBuffer(stagingBuffer, _vkVertexBuffer, bufferSize);

	// destroy staging buffer
	GAllocator->DestroyBuffer(stagingBuffer);
}

void Renderer::CreateIndexBuffer(std::vector<uint32> indices)
{
	uint64 numIndices = static_cast<uint64>(indices.size());
	uint64 perIndexSize = static_cast<uint64>(sizeof(indices[0]));
	VkDeviceSize bufferSize = numIndices * perIndexSize;

	VkBufferAllocated stagingBuffer;
	GAllocator->CreateBuffer(
		&stagingBuffer,
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VMA_MEMORY_USAGE_AUTO, 
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		"index staging buffer"
	);

	memcpy(stagingBuffer.allocationInfo.pMappedData, indices.data(), (size_t)(bufferSize));

	GAllocator->CreateBuffer(
		&_vkIndexBuffer,
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, 
		VMA_ALLOCATION_CREATE_MAPPED_BIT, 
		"index buffer"
	);

	// copy staging buffer to index buffer
	CopyBufferToBuffer(stagingBuffer, _vkIndexBuffer, bufferSize);

	// destroy staging buffer
	GAllocator->DestroyBuffer(stagingBuffer);
}

void Renderer::CreateDescriptorSet()
{
	// create descriptor set layout with bindings
	GDescriptorManager->CreateDescriptorSetLayout(_vkDescriptorSetLayout);
	// allocate descriptor set layout
	GDescriptorManager->AllocateDescriptorSet(_vkDescriptorSets, _vkDescriptorSetLayout);
}

void Renderer::AppendDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32 count)
{
	_descriptorTypeMap.insert(std::make_pair(type, _bindingOffset));
	GDescriptorManager->AddDescriptorSetLayoutBinding(type, stageFlags, _bindingOffset++, count);
}

void Renderer::CreateUniformBuffers()
{
	VkDeviceSize perUniformBufferSize = sizeof(UniformBufferObject);
	_vkUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t it = 0; it < MAX_FRAMES_IN_FLIGHT; it++)
	{
		GAllocator->CreateBuffer(
			&_vkUniformBuffers[it],
			perUniformBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			VMA_ALLOCATION_CREATE_MAPPED_BIT,
			"uniform buffer(" + std::to_string(it) + ")"
		);
	}
}

void Renderer::CreatePushConstantRaster()
{
	// initialize values in push constant raster
#ifdef USE_HLSL
	_vkPushConstantRaster.modelMat = XMMatrixIdentity();
	_vkPushConstantRaster.lightPosition = XMVectorSet(4.0f, 10.0f, 2.0f, 0.0f);
#else
	_vkPushConstantRaster.modelMat = glm::mat4(1.0f);
	_vkPushConstantRaster.lightPosition = glm::vec3(2.0f, 2.0f, 2.0f);
#endif
	_vkPushConstantRaster.lightIntensity = 1.0f;
	_vkPushConstantRaster.lightType = LightType::POINT_LIGHT;

	// create push constant range and insert it into ranges
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(VkPushConstantRaster);

	_vkPushConstantRanges.push_back(pushConstantRange);
}

void Renderer::UpdateUniformBuffer()
{
	UniformBufferObject ubo{};

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

	// update uniform buffer object
	// should not use GetMappedData() because update funciton requires state change.
	memcpy(_vkUniformBuffers[_currentFrameIndex].allocationInfo.pMappedData, &ubo, sizeof(ubo));
}

void Renderer::UpdateModelDescriptor()
{
	for (size_t it = 0; it < MAX_FRAMES_IN_FLIGHT; it++)
	{
		// write buffer descriptor
		GDescriptorManager->WriteBufferToDescriptorSet(
			_vkUniformBuffers[it].buffer,                          // uniform buffer
			0,                                                     // offset
			sizeof(UniformBufferObject),                           // range
			_descriptorTypeMap[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER], // binding point
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER                      // descriptor type
		);

		// texture image view and sampler is used commonly in descriptor sets
		GDescriptorManager->WriteImageToDescriptorSet(
			_vikingModel.vikingTexture.imageView,                          // texture image view
			_vikingModel.vikingTexture.sampler,                            // texture sampler
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                      // image layout
			_descriptorTypeMap[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER], // binding point
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER                      // descriptor type
		);

		// update descriptor set
		GDescriptorManager->UpdateDescriptorSet(_vkDescriptorSets[it]);
	}
}

void Renderer::Update()
{
	// timer update
	_timer.Update();

	// update input states
	_inputController.Update(_timer.deltaTime);

	// update camera
	_camera.UpdateViewTarget();

	// update uniform buffer object
	UpdateUniformBuffer();
}

void Renderer::CopyBufferToBuffer(VkBufferAllocated src, VkBufferAllocated dst, VkDeviceSize sz)
{
	// a command to copy buffer to buffer.
	GCommandService->ExecuteSingleTimeCommands([&](VkCommandBuffer commandBuffer) { // getting reference parameter outer-scope
		VkBufferCopy copyRegion{};
		copyRegion.size = sz;
		vkCmdCopyBuffer(commandBuffer, src.buffer, dst.buffer, 1, &copyRegion);
	});
}

void Renderer::RecordFrameBufferCommands(uint32 swapchainImageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;					// Optional
	beginInfo.pInheritanceInfo = nullptr;	// Optional

	// get rendering resource from pipeline
	MKGraphicsPipeline::RenderingResource& renderingResource = _mkGraphicsPipeline.GetRenderingResource(_currentFrameIndex);

	auto commandBuffer = *(renderingResource.commandBuffer);
	MK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	// 4. prepare render pass begin info
	auto swapchainExtent = _mkSwapchain.GetSwapchainExtent(); // store swapchain extent for common usage.


	const auto clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // settings for VK_ATTACHMENT_LOAD_OP_CLEAR in color attachment
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0] = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]} };   // clear values for color
	clearValues[1] = { 1.0f, 0 };                                                        // clear value for depth and stencil attachment

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = _vkRenderPass;
	renderPassBeginInfo.framebuffer = _mkSwapchain.GetFramebuffer(swapchainImageIndex);
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = swapchainExtent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	// 5. begin render pass
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// 6. set viewport and scissor for drawing
	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(swapchainExtent.width);
	viewport.height   = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// 7. bind graphics pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _mkGraphicsPipeline.GetPipeline()); // bind graphics pipeline
	
	// 8. bind descriptor sets
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_mkGraphicsPipeline.GetPipelineLayout(),
		0,
		1,
		&_vkDescriptorSets[_currentFrameIndex], // number of descriptor sets should fit into MAX_FRAMES_IN_FLIGHT
		0,
		nullptr
	);

	// 9. bind push constants
	uint32 pushConstantOffset = 0;
	uint32 pushConstantSize = sizeof(VkPushConstantRaster);
	vkCmdPushConstants(
		commandBuffer,
		_mkGraphicsPipeline.GetPipelineLayout(),
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		pushConstantOffset,
		pushConstantSize,
		&_vkPushConstantRaster
	);

	// 10. bind vertex and index buffer
	VkBuffer vertexBuffers[] = { _vkVertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);					// bind vertex buffer
	vkCmdBindIndexBuffer(commandBuffer, _vkIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);    // bind index buffer

	// 11. record draw command
	vkCmdDrawIndexed(commandBuffer, _vikingModel.indices.size(), 1, 0, 0, 0);

	// 12. end up render pass
	vkCmdEndRenderPass(commandBuffer);

	MK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void Renderer::Rasterize()
{
	// update every states
	Update();

	// 1. wait for the previous frame to be finished
	MKGraphicsPipeline::RenderingResource& renderingResource = _mkGraphicsPipeline.GetRenderingResource(_currentFrameIndex);
	vkWaitForFences(_mkDevice.GetDevice(), 1, &renderingResource.inFlightFence, VK_TRUE, UINT64_MAX);

	// 2. get available image from swapchain
	uint32 imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		_mkDevice.GetDevice(), 
		_mkSwapchain.GetSwapchain(), 
		UINT64_MAX, 
		renderingResource.imageAvailableSema, 
		VK_NULL_HANDLE, 
		&imageIndex
	);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) // 2.1 error by out of date -> recreate swapchain
	{
		_mkSwapchain.RecreateSwapchain(_vkRenderPass);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) // 2. error by another reason
	{
		MK_THROW("Failed to acquire swap chain image!.")
	}

	// 3. reset fence if and only if application is submitting commands
	vkResetFences(_mkDevice.GetDevice(), 1, &renderingResource.inFlightFence); // make current fence unsignaled

	// 4. reset frame buffer command buffer
	GCommandService->ResetCommandBuffer(_currentFrameIndex);

	// 5. record frame buffer commands
	RecordFrameBufferCommands(imageIndex);

	// 6. copy rendering resources and submit recorded command buffer to graphics queue
	VkSemaphore          waitSemaphores[]   = { renderingResource.imageAvailableSema };
	VkPipelineStageFlags waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore          signalSemaphores[] = { renderingResource.renderFinishedSema };

	GCommandService->SubmitCommandBufferToQueue(
		_currentFrameIndex,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		_mkDevice.GetGraphicsQueue(),
		renderingResource.inFlightFence
	);

	// 7. present image to swapchain
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = signalSemaphores; // use same signal semaphore to wait on command buffer to finish execution

	// specify the swap chains to present images to and the index of the image for each swap chain
	VkSwapchainKHR swapChains[] = { _mkSwapchain.GetSwapchain() };
	presentInfo.swapchainCount  = 1;
	presentInfo.pSwapchains     = swapChains;
	presentInfo.pImageIndices   = &imageIndex;
	presentInfo.pResults        = nullptr;

	result = vkQueuePresentKHR(_mkDevice.GetPresentQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) // 7.1 error by out of date or suboptimal -> recreate swapchain
	{
		_mkSwapchain.RequestFramebufferResize(false); // framebuffer resize is not required
		_mkSwapchain.RecreateSwapchain(_vkRenderPass);
	}
	else if (result != VK_SUCCESS)
		MK_THROW("failed to present swap chain image!");

	// 8. update current frame index
	_currentFrameIndex = (_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::Render()
{
	while (!_mkWindow.ShouldClose()) 
	{
		_mkWindow.PollEvents();
		Rasterize();
	}

	_mkDevice.WaitUntilDeviceIdle();
}
