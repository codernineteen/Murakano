#include "Renderer.h"

Renderer::Renderer()
	: 
	_mkWindow(true), 
	_mkInstance(), 
	_mkDevice(_mkWindow, _mkInstance), 
	_mkSwapchain(_mkDevice),
	_mkGraphicsPipeline(_mkDevice, _mkSwapchain),
	_objModel(_mkDevice),
	_camera(_mkDevice, _mkSwapchain),
	_inputController(_mkWindow.GetWindow(), _camera)
{
	GAllocator->InitVMAAllocator(_mkInstance.GetVkInstance(), _mkDevice.GetPhysicalDevice(), _mkDevice.GetDevice());
	
	// get device physical properties for later use
	vkGetPhysicalDeviceProperties(_mkDevice.GetPhysicalDevice(), &_vkDeviceProperties);

}

Renderer::~Renderer()
{
	// destroy buffers
	GAllocator->DestroyBuffer(_vkVertexBuffer);
	GAllocator->DestroyBuffer(_vkIndexBuffer);

	for(auto& uniformBuffer : _vkUniformBuffers)
		GAllocator->DestroyBuffer(uniformBuffer);

	// destroy image sampler
	vkDestroySampler(_mkDevice.GetDevice(), _vkLinearSampler, nullptr);

	// destroy model and texture resources
	_objModel.DestroyModel();

	// destroy descriptor set layout
	vkDestroyDescriptorSetLayout(_mkDevice.GetDevice(), _vkBaseDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(_mkDevice.GetDevice(), _vkSamplerDescriptorSetLayout, nullptr);

	// destroy frame buffers
	DestroyFrameBuffers();

	// destroy render pass
	vkDestroyRenderPass(_mkDevice.GetDevice(), _vkRenderPass, nullptr);

	// destroy offscreen render pass resources
	DestroyOffscreenRenderPassResources();

	// destroy allocator instance
	delete GAllocator;
#ifndef NDEBUG
	MK_LOG("descriptor set layout destroyed");
#endif
}

/**
* ----------------- Initialization -----------------
*/

void Renderer::Setup()
{
	// query required color & depth attachment formats
	VkFormat swapchainImageFormat = _mkSwapchain.GetSwapchainImageFormat(); // color attachment format
	VkFormat depthFormat = mk::vk::FindDepthFormat(_mkDevice.GetPhysicalDevice()); // depth attachment format
	
	// create render pass
	mk::vk::CreateDefaultRenderPass(_mkDevice.GetDevice(), swapchainImageFormat, depthFormat, &_vkRenderPass);

	// request creation of frame buffers
	CreateFrameBuffers();

	// create linear filtering mode image sampler
	mk::vk::CreateSampler(_mkDevice.GetDevice(), &_vkLinearSampler, _vkDeviceProperties);

	// prepare model resources
	
	// for head rendering
	_objModel.LoadModel(
		"../../../resources/Models/head_model.obj",
		{
			{"diffuse texture", "../../../resources/Textures/head_diffuse.png"},  // diffuse
			{"specular texture", "../../../resources/Textures/head_specular.png"}, // specular
			{"normal map", "../../../resources/Textures/head_normal.png"}          // normal
		}
	);
	
	// for viking room rendering
	//_objModel.LoadModel(
	//	"../../../resources/Models/viking_room.obj",
	//	{
	//		{"diffuse texture", "../../../resources/Textures/viking_room.png"},  // diffuse
	//	}
	//);

	// transform model if needed
	_objModel.Scale(0.05f, 0.05f, 0.05f); // scale down
	_objModel.RotateX(90.0f);             // rotate x-axis
	_objModel.RotateY(90.0f);             // rotate y-axis

	// create vertex buffer
	CreateVertexBuffer(_objModel.vertices);

	// create index buffer
	CreateIndexBuffer(_objModel.indices);

	// create push constant for rasterization
	CreatePushConstantRaster();

	// create uniform buffers
	CreateUniformBuffers();

	// create required descriptor sets
	CreateBaseDescriptorSet();
	CreateSamplerDescriptorSet();

	// update model descriptor set
	WriteBaseDescriptor();
	
	// update sampler descriptor set
	WriteSamplerDescriptor();

	// create offscreen render pass
	//CreateOffscreenRenderPass();

	// build graphics pipeline
	std::vector<VkDescriptorSetLayout> layouts = { _vkBaseDescriptorSetLayout, _vkSamplerDescriptorSetLayout };
	_mkGraphicsPipeline.BuildPipeline(layouts, _vkPushConstantRanges, _vkRenderPass);
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

void Renderer::CreateFrameBuffers()
{
	auto imageViewCount = _mkSwapchain.GetImageViewCount();
	_vkFramebuffers.resize(imageViewCount);

	// create frame buffer as much as the number of image views
	for (size_t it = 0; it < imageViewCount; it++) {
		
		std::array<VkImageView, 2> attachments = {
			_mkSwapchain.GetSwapchainImageView(it), // only color image views are different for each frame buffer
			_mkSwapchain.GetDepthImageView(),
		};

		VkFramebufferCreateInfo framebufferInfo = vkinfo::GetFramebufferCreateInfo(_vkRenderPass, attachments, _mkSwapchain.GetSwapchainExtent());
		MK_CHECK(vkCreateFramebuffer(_mkDevice.GetDevice(), &framebufferInfo, nullptr, &_vkFramebuffers[it]));
	}
}

void Renderer::CreateBaseDescriptorSet()
{
	// single uniform buffer descriptor
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT,
		EVertexShaderBinding::UNIFORM_BUFFER,
		1
	);
	// three texture image descriptors
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		EFragmentShaderBinding::TEXTURE,
		static_cast<uint32>(_objModel.textures.size())
	);


	// create base descriptor set layout based on waiting bindings
	GDescriptorManager->CreateDescriptorSetLayout(_vkBaseDescriptorSetLayout);

	// allocate base descriptor set
	GDescriptorManager->AllocateDescriptorSet(_vkBaseDescriptorSets, _vkBaseDescriptorSetLayout);
}

void Renderer::CreateOffscreenRenderPass()
{
	if (_vkOffscreenColorImage.image != VK_NULL_HANDLE)
	{
		GAllocator->DestroyImage(_vkOffscreenColorImage);
		vkDestroyImageView(_mkDevice.GetDevice(), _vkOffscreenColorImageView, nullptr);
		vkDestroySampler(_mkDevice.GetDevice(), _vkOffscreenColorSampler, nullptr);
	}

	if (_vkOffscreenDepthImage.image != VK_NULL_HANDLE)
	{
		GAllocator->DestroyImage(_vkOffscreenDepthImage);
		vkDestroyImageView(_mkDevice.GetDevice(), _vkOffscreenDepthImageView, nullptr);
	}

	/**
	* specify color image usage
	* - transfer source : for copying image to other image
	* - transfer destination : for copying image from other image
	* - color attachment : framebuffer resolver
	* - sampled image : make image view suitable for texture sampling
	* - storage image : make image view suitable for storage image
	*/
	auto transferUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	auto colorImageUsages = transferUsages | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	auto depthImageUsages = transferUsages | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	// create color image
	GAllocator->CreateImage(
		&_vkOffscreenColorImage,
		WIDTH, HEIGHT,
		_vkOffscreenColorFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		colorImageUsages, // for storage image, sampler and framebuffer resolver
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED
	);
	
	// create color image view
	mk::vk::CreateImageView(
		_mkDevice.GetDevice(),
		_vkOffscreenColorImage.image,
		_vkOffscreenColorImageView,
		_vkOffscreenColorFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_REMAINING_MIP_LEVELS
	);
	
	// create offscreen color sampler
	VkSamplerCreateInfo samplerInfo{};
	mk::vk::CreateSampler(_mkDevice.GetDevice(), &_vkOffscreenColorSampler, _vkDeviceProperties);

	// create depth image
	GAllocator->CreateImage(
		&_vkOffscreenDepthImage,
		WIDTH, HEIGHT,
		_vkOffscreenDepthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		depthImageUsages,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED
	);

	// create color image view
	mk::vk::CreateImageView(
		_mkDevice.GetDevice(),
		_vkOffscreenDepthImage.image,
		_vkOffscreenDepthImageView,
		_vkOffscreenDepthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1, // mip levels
		1  // layer count
	);

	// record transition image layout commands
	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBuffer cmdBuffer;
	GCommandService->BeginSingleTimeCommands(cmdBuffer, cmdPool);
	mk::vk::TransitionImageLayout(
		cmdBuffer,
		_vkOffscreenColorImage.image,
		_vkOffscreenColorFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
	);

	mk::vk::TransitionImageLayout(
		cmdBuffer,
		_vkOffscreenDepthImage.image,
		_vkOffscreenDepthFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
	);

	GCommandService->EndSingleTimeCommands(cmdBuffer);

	// create offscreen render pass
	if (_vkOffscreenRednerPass == VK_NULL_HANDLE)
	{
		_vkOffscreenRednerPass = mk::vk::CreateRenderPass(
			_mkDevice.GetDevice(),
			{ _vkOffscreenColorFormat },
			_vkOffscreenDepthFormat,
			1,                       // subpass count
			true,                    // use clear color
			true,                    // use clear depth
			VK_IMAGE_LAYOUT_GENERAL, // initial layout
			VK_IMAGE_LAYOUT_GENERAL  // final layout
		);
	}

	// create offscreen frame buffer
	std::array<VkImageView, 2> attachments = { _vkOffscreenColorImageView, _vkOffscreenDepthImageView };
	
	// destroy framebuffer
	vkDestroyFramebuffer(_mkDevice.GetDevice(), _vkOffscreenFramebuffer, nullptr);

	VkExtent2D extent = { WIDTH, HEIGHT };
	VkFramebufferCreateInfo framebufferInfo = vkinfo::GetFramebufferCreateInfo(_vkOffscreenRednerPass, attachments, extent);
	
	// create framebuffer
	MK_CHECK(vkCreateFramebuffer(_mkDevice.GetDevice(), &framebufferInfo, nullptr, &_vkOffscreenFramebuffer));
}

void Renderer::CreateSamplerDescriptorSet()
{
	// single sampler descriptor
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		EFragmentShaderBinding::SAMPLER,
		1
	);

	// create sampler descriptor set layout based on waiting bindings
	GDescriptorManager->CreateDescriptorSetLayout(_vkSamplerDescriptorSetLayout);

	// allocate sampler descriptor set
	GDescriptorManager->AllocateDescriptorSet(_vkSamplerDescriptorSets, _vkSamplerDescriptorSetLayout);
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
	_vkPushConstantRaster.lightPosition = XMVectorSet(12.0f, 11.0f, 0.0f, 0.0f);
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

/**
* ----------------- Destroy -----------------
*/
void Renderer::DestroyOffscreenRenderPassResources()
{
	// destroy offscreen render pass
	vkDestroyRenderPass(_mkDevice.GetDevice(), _vkOffscreenRednerPass, nullptr);

	// destroy offscreen color image
	GAllocator->DestroyImage(_vkOffscreenColorImage);

	// destroy offscreen color image view
	vkDestroyImageView(_mkDevice.GetDevice(), _vkOffscreenColorImageView, nullptr);

	// destroy offscreen color sampler
	vkDestroySampler(_mkDevice.GetDevice(), _vkOffscreenColorSampler, nullptr);

	// destroy offscreen depth image
	GAllocator->DestroyImage(_vkOffscreenDepthImage);

	// destroy offscreen depth image view
	vkDestroyImageView(_mkDevice.GetDevice(), _vkOffscreenDepthImageView, nullptr);

	// destroy offscreen framebuffer
	vkDestroyFramebuffer(_mkDevice.GetDevice(), _vkOffscreenFramebuffer, nullptr);
}

void Renderer::DestroyFrameBuffers()
{
	for (auto framebuffer : _vkFramebuffers)
		vkDestroyFramebuffer(_mkDevice.GetDevice(), framebuffer, nullptr);
}

/**
* ----------------- Update -----------------
*/

void Renderer::UpdateUniformBuffer()
{
	UniformBufferObject ubo{};

#ifdef USE_HLSL
	auto projViewMat = _camera.GetProjectionMatrix() * _camera.GetViewMatrix();

	// initialize model transformation
	auto modelMat = _objModel.GetModelMatrix();

	// Because SIMD operation is supported, i did multiplication in application side, not in shader side.
	ubo.mvpMat = projViewMat * modelMat;
#else
	// fill out uniform buffer object members
	ubo.modelMat = _objModel.GetModelMatrix();
	ubo.viewMat = _camera.GetViewMatrix();
	ubo.projMat = _camera.GetProjectionMatrix();
#endif

	// update uniform buffer object
	// should not use GetMappedData() because update funciton requires state change.
	memcpy(_vkUniformBuffers[_currentFrameIndex].allocationInfo.pMappedData, &ubo, sizeof(ubo));
}

void Renderer::WriteBaseDescriptor()
{
	for (size_t it = 0; it < MAX_FRAMES_IN_FLIGHT; it++)
	{
		// write buffer descriptor
		GDescriptorManager->WriteBufferToDescriptorSet(
			_vkUniformBuffers[it].buffer,         // uniform buffer
			0,                                    // offset
			sizeof(UniformBufferObject),          // range
			EVertexShaderBinding::UNIFORM_BUFFER, // binding point
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER     // descriptor type
		);

		// texture image descriptor
		// - store a set of image infos to write at once
		std::vector<VkDescriptorImageInfo> imageInfos; 
		for (size_t tex_it = 0; tex_it < _objModel.textures.size(); tex_it++) 
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView   = _objModel.textures[tex_it]->imageView;
			imageInfo.sampler     = nullptr;
			imageInfos.push_back(imageInfo);
		}
		// - call image array write api 
		GDescriptorManager->WriteImageArrayToDescriptorSet(
			imageInfos.data(),
			static_cast<uint32>(imageInfos.size()),
			EFragmentShaderBinding::TEXTURE,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		);

		// update base descriptor set per frame
		GDescriptorManager->UpdateDescriptorSet(_vkBaseDescriptorSets[it]);
	}
}

void Renderer::WriteSamplerDescriptor()
{
	for (size_t it = 0; it < MAX_FRAMES_IN_FLIGHT; it++)
	{
		// write sampler descriptor
		GDescriptorManager->WriteSamplerToDescriptorSet(
			_vkLinearSampler,
			EFragmentShaderBinding::SAMPLER,
			VK_DESCRIPTOR_TYPE_SAMPLER
		);

		// update sampler descriptor set
		GDescriptorManager->UpdateDescriptorSet(_vkSamplerDescriptorSets[it]);
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

void Renderer::OnResizeWindow()
{
	// handling window minimization
	int width = 0, height = 0;
	glfwGetFramebufferSize(_mkWindow.GetWindow(), &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(_mkWindow.GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	// wait until the device is idle
	vkDeviceWaitIdle(_mkDevice.GetDevice()); 

	// destroy resources first
	_mkSwapchain.DestroySwapchainResources();
	DestroyFrameBuffers();

	// recreate swapchain
	_mkSwapchain.CreateSwapchain();
	_mkSwapchain.CreateSwapchainImageViews();
	_mkSwapchain.CreateDepthResources();
	CreateFrameBuffers();
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


	const auto clearColor = glm::vec4(0.01f, 0.01f, 0.01f, 1.0f); // settings for VK_ATTACHMENT_LOAD_OP_CLEAR in color attachment
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0] = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]} };   // clear values for color
	clearValues[1] = { 1.0f, 0 };                                                        // clear value for depth and stencil attachment

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = _vkRenderPass;
	renderPassBeginInfo.framebuffer = _vkFramebuffers[swapchainImageIndex];
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
	
	// 8. bind base descriptor sets
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_mkGraphicsPipeline.GetPipelineLayout(),
		0,                                          // set index 0
		1,
		&_vkBaseDescriptorSets[_currentFrameIndex], // number of descriptor sets should fit into MAX_FRAMES_IN_FLIGHT
		0,
		nullptr
	);

	// 9. bind sampler descriptor set
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_mkGraphicsPipeline.GetPipelineLayout(),
		1,                                             // set index 1
		1,
		&_vkSamplerDescriptorSets[_currentFrameIndex], // number of descriptor sets should fit into MAX_FRAMES_IN_FLIGHT
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
	vkCmdDrawIndexed(commandBuffer, _objModel.indices.size(), 1, 0, 0, 0);

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
		OnResizeWindow();
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
		_mkDevice.SetFrameBufferResized(false);
		OnResizeWindow();
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
