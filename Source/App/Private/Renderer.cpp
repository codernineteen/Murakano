#include "Renderer.h"

Renderer::Renderer()
	: 
	_mkWindow(true), 
	_mkInstance(), 
	_mkDevice(_mkWindow, _mkInstance), 
	_mkSwapchain(_mkDevice),
	_mkPostPipeline(_mkDevice, "post process"),
	_metalRoughPipeline(_mkDevice),
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
	vkDestroyDescriptorSetLayout(_mkDevice.GetDevice(), _vkGlobalDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(_mkDevice.GetDevice(), _vkPostDescriptorSetLayout, nullptr);

	// destroy sync objects
	for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(_mkDevice.GetDevice(), _drawSyncObjects[i].renderFinishedSema, nullptr);
		vkDestroySemaphore(_mkDevice.GetDevice(), _drawSyncObjects[i].imageAvailableSema, nullptr);
		vkDestroyFence(_mkDevice.GetDevice(), _drawSyncObjects[i].inFlightFence, nullptr);
	}


	if (_mkDevice.enableDynamicRendering)
	{
		// destroy offscreen rendering resources
		DestroyOffscreenRenderingResources();
	}
	else
	{
		// destroy offscreen render pass resources
		DestroyOffscreenRenderingResources();
		DestroyOffscreenRenderPassResources();

		// destroy frame buffers
		DestroyFrameBuffers();

		// destroy render pass
		vkDestroyRenderPass(_mkDevice.GetDevice(), _vkRenderPass, nullptr);
	}

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
	VkFormat swapchinDepthFormat = mk::vk::FindDepthFormat(_mkDevice.GetPhysicalDevice()); // depth attachment format
	
	if (_mkDevice.enableDynamicRendering)
	{
		// create offscreen rendering resources (color image and its view / depth image and its view / color sampler)
		CreateOffscreenRenderResource(_mkSwapchain.GetSwapchainExtent());
	}
	else
	{
		// create render pass
		mk::vk::CreateDefaultRenderPass(_mkDevice.GetDevice(), swapchainImageFormat, swapchinDepthFormat, &_vkRenderPass);
		// request creation of frame buffers
		CreateFrameBuffers();
		// create offscreen render pass
		CreateOffscreenRenderPass({WIDTH, HEIGHT});
	}


	// create linear filtering mode image sampler
	mk::vk::CreateSampler(_mkDevice.GetDevice(), &_vkLinearSampler, _vkDeviceProperties);
	
	// for head rendering
	_objModel.LoadModel(
		"../../../resources/Models/head_model.obj",
		{
			{"diffuse texture", "../../../resources/Textures/head_diffuse.png"},  // diffuse
			{"specular texture", "../../../resources/Textures/head_specular.png"}, // specular
			{"normal map", "../../../resources/Textures/head_normal.png"}          // normal
		}
	);

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

	// create descriptor sets for graphics pipeline
	CreateGlobalDescriptorSet();
	WriteGlobalDescriptor();  // update model descriptor set

	// create descriptor set for post processing pipeline
	CreatePostDescriptorSet();
	WritePostDescriptor(); // update post descriptor set

	// create synchronization objects
	CreateSyncObjects();

	// determine stencil format for two pipelines
	auto offscreenStencilFormat = (!IsDepthOnlyFormat(_vkOffscreenDepthFormat)) ? _vkOffscreenDepthFormat : VK_FORMAT_UNDEFINED;
	auto swapchainStencilFormat = (!IsDepthOnlyFormat(swapchinDepthFormat)) ? swapchinDepthFormat : VK_FORMAT_UNDEFINED;

	// describe descriptor set layout binding and allocate descriptor sets
	_metalRoughPipeline.CreateDescriptorSet(static_cast<uint32>(_objModel.textures.size()));
	// allocate memory for material buffer and write material data
	CreateMetallicRoughnessMaterialBuffer();   
	// create material pipeline
	_metalRoughPipeline.BuildPipeline(
		_vkPushConstantRanges,
		_vkGlobalDescriptorSetLayout, 
		{ _vkOffscreenColorFormat },
		_vkOffscreenDepthFormat
	);

	// configure post pipeline
	_mkPostPipeline.AddShader("../../../shaders/output/spir-v/post-vertex.spv", "main", VK_SHADER_STAGE_VERTEX_BIT);
	_mkPostPipeline.AddShader("../../../shaders/output/spir-v/post-fragment.spv", "main", VK_SHADER_STAGE_FRAGMENT_BIT);
	_mkPostPipeline.SetDefaultPipelineCreateInfo();

	// configure post pipeline layout
	std::vector<VkDescriptorSetLayout> postDescriptorLayouts = { _vkPostDescriptorSetLayout };
	_mkPostPipeline.AddDescriptorSetLayouts(postDescriptorLayouts);
	_mkPostPipeline.CreatePipelineLayout();
	_mkPostPipeline.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE); // disable culling
	_mkPostPipeline.RemoveVertexInput();

	if (_mkDevice.enableDynamicRendering)
	{
		_mkPostPipeline.SetRenderingInfo(1, &swapchainImageFormat, swapchinDepthFormat, swapchainStencilFormat);
		_mkPostPipeline.BuildPipeline();
	}
	else
	{
		_mkPostPipeline.BuildPipeline(&_vkRenderPass);
	}
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

		VkFramebufferCreateInfo framebufferInfo = mk::vkinfo::GetFramebufferCreateInfo(_vkRenderPass, attachments, _mkSwapchain.GetSwapchainExtent());
		MK_CHECK(vkCreateFramebuffer(_mkDevice.GetDevice(), &framebufferInfo, nullptr, &_vkFramebuffers[it]));
	}
}

void Renderer::CreateGlobalDescriptorSet()
{
	// single uniform buffer descriptor
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_VS_FS_FLAG,
		EShaderBinding::UNIFORM_BUFFER,
		1
	);

	// create base descriptor set layout based on waiting bindings
	GDescriptorManager->CreateDescriptorSetLayout(_vkGlobalDescriptorSetLayout);

	// allocate base descriptor set
	GDescriptorManager->AllocateDescriptorSet(_vkGlobalDescriptorSets, _vkGlobalDescriptorSetLayout);
}

void Renderer::CreatePostDescriptorSet()
{
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		EShaderBinding::COMBINED_IMAGE_SAMPLER, // binding point
		1  // count
	);

	GDescriptorManager->CreateDescriptorSetLayout(_vkPostDescriptorSetLayout);
	GDescriptorManager->AllocateDescriptorSet(_vkPostDescriptorSets, _vkPostDescriptorSetLayout);
}

void Renderer::CreateOffscreenRenderResource(VkExtent2D extent)
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
	bool isTransferRequired = false;
	VkImageUsageFlags transferUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkImageUsageFlags colorImageUsages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	VkImageUsageFlags depthImageUsages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (isTransferRequired)
	{
		colorImageUsages |= transferUsages;
		depthImageUsages |= transferUsages;
	}

	// create color image
	GAllocator->CreateImage(
		&_vkOffscreenColorImage,
		extent.width, extent.height,
		_vkOffscreenColorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		colorImageUsages, // for storage image, sampler and framebuffer resolver
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		"offscreen color image"
	);

	// create color image view
	mk::vk::CreateImageView(
		_mkDevice.GetDevice(),
		_vkOffscreenColorImage.image,
		_vkOffscreenColorImageView,
		VK_IMAGE_VIEW_TYPE_2D,
		_vkOffscreenColorFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_REMAINING_MIP_LEVELS
	);

	// create offscreen color sampler
	mk::vk::CreateSampler(_mkDevice.GetDevice(), &_vkOffscreenColorSampler, _vkDeviceProperties);

	// create offscreen color sampler
	if (_vkOffscreenColorSampler == VK_NULL_HANDLE)
	{
		mk::vk::CreateSampler(_mkDevice.GetDevice(), &_vkOffscreenColorSampler, _vkDeviceProperties);
	}

	// create depth image
	GAllocator->CreateImage(
		&_vkOffscreenDepthImage,
		extent.width, extent.height,
		_vkOffscreenDepthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		depthImageUsages,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		"offscreen depth image"
	);

	// create depth image view
	VkImageAspectFlags depthAspectFlags = (_vkOffscreenDepthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;

	mk::vk::CreateImageView(
		_mkDevice.GetDevice(),
		_vkOffscreenDepthImage.image,
		_vkOffscreenDepthImageView,
		VK_IMAGE_VIEW_TYPE_2D,
		_vkOffscreenDepthFormat,
		depthAspectFlags,
		1, // mip levels
		1  // layer count
	);

	// record transition image layout commands
	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBuffer commandBufferfer;
	GCommandService->BeginSingleTimeCommands(commandBufferfer, cmdPool);

	mk::vk::TransitionImageLayout(
		commandBufferfer,
		_vkOffscreenColorImage.image,
		_vkOffscreenColorFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
	);

	mk::vk::TransitionImageLayout(
		commandBufferfer,
		_vkOffscreenDepthImage.image,
		_vkOffscreenDepthFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		{ depthAspectFlags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
	);

	GCommandService->EndSingleTimeCommands(commandBufferfer, cmdPool);
}

void Renderer::CreateOffscreenRenderPass(VkExtent2D extent)
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
		extent.width, extent.height,
		_vkOffscreenColorFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		colorImageUsages, // for storage image, sampler and framebuffer resolver
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		"offscreen color image"
	);
	
	// create color image view
	mk::vk::CreateImageView(
		_mkDevice.GetDevice(),
		_vkOffscreenColorImage.image,
		_vkOffscreenColorImageView,
		VK_IMAGE_VIEW_TYPE_2D,
		_vkOffscreenColorFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_REMAINING_MIP_LEVELS
	);
	
	// create offscreen color sampler
	if (_vkOffscreenColorSampler == VK_NULL_HANDLE)
	{
		mk::vk::CreateSampler(_mkDevice.GetDevice(), &_vkOffscreenColorSampler, _vkDeviceProperties);
	}
	
	// create depth image
	GAllocator->CreateImage(
		&_vkOffscreenDepthImage,
		extent.width, extent.height,
		_vkOffscreenDepthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		depthImageUsages,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		"offscreen depth image"
	);

	// create color image view
	mk::vk::CreateImageView(
		_mkDevice.GetDevice(),
		_vkOffscreenDepthImage.image,
		_vkOffscreenDepthImageView,
		VK_IMAGE_VIEW_TYPE_2D,
		_vkOffscreenDepthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1, // mip levels
		1  // layer count
	);

	// record transition image layout commands
	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBuffer commandBufferfer;
	GCommandService->BeginSingleTimeCommands(commandBufferfer, cmdPool);

	mk::vk::TransitionImageLayout(
		commandBufferfer,
		_vkOffscreenColorImage.image,
		_vkOffscreenColorFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
	);

	mk::vk::TransitionImageLayout(
		commandBufferfer,
		_vkOffscreenDepthImage.image,
		_vkOffscreenDepthFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS }
	);

	GCommandService->EndSingleTimeCommands(commandBufferfer, cmdPool);

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
	if(_vkOffscreenFramebuffer != VK_NULL_HANDLE)
		vkDestroyFramebuffer(_mkDevice.GetDevice(), _vkOffscreenFramebuffer, nullptr);
	VkFramebufferCreateInfo framebufferInfo = mk::vkinfo::GetFramebufferCreateInfo(_vkOffscreenRednerPass, attachments, extent);
	
	// create framebuffer
	MK_CHECK(vkCreateFramebuffer(_mkDevice.GetDevice(), &framebufferInfo, nullptr, &_vkOffscreenFramebuffer));
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

void Renderer::CreateMetallicRoughnessMaterialBuffer()
{
	// allocate memory for material buffer
	VkBufferAllocated materialConstantBuffer;
	GAllocator->CreateBuffer(
		&materialConstantBuffer,
		sizeof(GLTFMetallicRoughnessPipeline::MaterialConstants),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		VMA_ALLOCATION_CREATE_MAPPED_BIT,
		"material buffer"
	);

	GLTFMetallicRoughnessPipeline::MaterialConstants constant{};
	constant.color = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	constant.metallicRoughness = XMVectorSet(1.0f, 0.5f, 0.0f, 0.0f);

	// copy local stack memory into material buffer
	memcpy(materialConstantBuffer.allocationInfo.pMappedData, &constant, sizeof(constant));
	GAllocator->PushDestruction([=,this]() {
		GAllocator->DestroyBuffer(materialConstantBuffer);
	});


	GLTFMetallicRoughnessPipeline::MaterialResources resources;
	resources.textureSampler = _vkLinearSampler;
	for (size_t it = 0; it < _objModel.textures.size(); it++)
	{
		resources.textureViews.push_back(_objModel.textures[it]->imageView);
	}
	resources.dataBuffer = materialConstantBuffer;
	resources.dataBufferOffset = 0;

	_defaultData = _metalRoughPipeline.WriteMaterial(EMaterialPass::OPAQUE, resources);
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
	_vkPushConstantRaster.lightType = ELightType::POINT_LIGHT;

	// create push constant range and insert it into ranges
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(VkPushConstantRaster);

	_vkPushConstantRanges.push_back(pushConstantRange);
}

void Renderer::CreateSyncObjects()
{
	_drawSyncObjects.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // create fence as signaled state for the very first frame.

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		MK_CHECK(vkCreateSemaphore(_mkDevice.GetDevice(), &semaphoreInfo, nullptr, &_drawSyncObjects[i].imageAvailableSema));
		MK_CHECK(vkCreateSemaphore(_mkDevice.GetDevice(), &semaphoreInfo, nullptr, &_drawSyncObjects[i].renderFinishedSema));
		MK_CHECK(vkCreateFence(_mkDevice.GetDevice(), &fenceInfo, nullptr, &_drawSyncObjects[i].inFlightFence));
	}
}

/**
* ----------------- Destroy -----------------
*/
void Renderer::DestroyOffscreenRenderingResources()
{
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
}

void Renderer::DestroyOffscreenRenderPassResources()
{
	// destroy offscreen render pass
	vkDestroyRenderPass(_mkDevice.GetDevice(), _vkOffscreenRednerPass, nullptr);

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
	// DirectXMath library supports SIMD operation
	
	// initialize model transformation
	auto modelMat = _objModel.GetModelMatrix();

	// assign transformations
	ubo.viewMat = _camera.GetViewMatrix();
	ubo.projMat = _camera.GetProjectionMatrix();
	ubo.viewInverseMat = XMMatrixInverse(nullptr, ubo.viewMat);
	ubo.mvpMat = ubo.projMat * ubo.viewMat * modelMat;
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

void Renderer::WriteGlobalDescriptor()
{
	for (size_t it = 0; it < MAX_FRAMES_IN_FLIGHT; it++)
	{
		// write buffer descriptor
		GDescriptorManager->WriteBufferToDescriptorSet(
			_vkUniformBuffers[it].buffer,         // uniform buffer
			0,                                    // offset
			sizeof(UniformBufferObject),          // range
			EShaderBinding::UNIFORM_BUFFER,       // binding point
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER     // descriptor type
		);

		// update base descriptor set per frame
		GDescriptorManager->UpdateDescriptorSet(_vkGlobalDescriptorSets[it]);
	}
}

void Renderer::WritePostDescriptor()
{
	for (size_t it = 0; it < MAX_FRAMES_IN_FLIGHT; it++)
	{
		GDescriptorManager->WriteCombinedImageSamplerToDescriptorSet(
			_vkOffscreenColorImageView,
			_vkOffscreenColorSampler,
			VK_IMAGE_LAYOUT_GENERAL,
			EShaderBinding::COMBINED_IMAGE_SAMPLER
		);
		
		GDescriptorManager->UpdateDescriptorSet(_vkPostDescriptorSets[it]);
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

	// recreate swapchain frame buffer
	CreateFrameBuffers();

	// recreate offscreen render pass (recreation of offscreen buffer included in here)
	auto extent = _mkSwapchain.GetSwapchainExtent();
	if (_mkDevice.enableDynamicRendering)
	{
		CreateOffscreenRenderResource(extent);
	}
	else
	{ // DEPRECATED
		CreateOffscreenRenderPass(extent);
	}
	// update post descriptor set because combined image sampler is dependent on offscreen color image view
	WritePostDescriptor();
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

/**
----------------- Draw -----------------
*/
void Renderer::Rasterize(const VkCommandBuffer& commandBuffer, VkExtent2D extent)
{
	// set viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// bind graphics pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, /*_mkGraphicsPipeline.GetPipeline()*/ _metalRoughPipeline.GetPipeline(EMaterialPass::OPAQUE)); // bind graphics pipeline

	// bind global descriptor sets
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_metalRoughPipeline.GetPipelineLayout(EMaterialPass::OPAQUE),
		0,                                            // set index 0
		1,                                            // count
		&_vkGlobalDescriptorSets[_currentFrameIndex], // number of descriptor sets should fit into MAX_FRAMES_IN_FLIGHT
		0,
		nullptr
	);

	// bind material descriptor set
	auto descSet = _metalRoughPipeline.GetDescriptorSet(_currentFrameIndex);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_metalRoughPipeline.GetPipelineLayout(EMaterialPass::OPAQUE),
		1,                                             // set index 1
		1,
		&descSet,
		0,
		nullptr
	);

	// bind push constants
	uint32 pushConstantOffset = 0;
	uint32 pushConstantSize = sizeof(VkPushConstantRaster);
	vkCmdPushConstants(
		commandBuffer,
		_metalRoughPipeline.GetPipelineLayout(EMaterialPass::OPAQUE),
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		pushConstantOffset,
		pushConstantSize,
		&_vkPushConstantRaster
	);

	// bind vertex and index buffer
	VkBuffer vertexBuffers[] = { _vkVertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);			      // bind vertex buffer
	vkCmdBindIndexBuffer(commandBuffer, _vkIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32); // bind index buffer

	// record draw command
	vkCmdDrawIndexed(commandBuffer, _objModel.indices.size(), 1, 0, 0, 0);
}

void Renderer::DrawPostProcess(const VkCommandBuffer& commandBuffer, VkExtent2D extent)
{
	// set viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
	
	vkCmdBindPipeline(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_mkPostPipeline.GetPipeline()
	);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		_mkPostPipeline.GetPipelineLayout(),
		0,
		1,
		&_vkPostDescriptorSets[_currentFrameIndex],
		0,
		nullptr
	);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0); // draw full quad
}


void Renderer::RecordFrameBufferCommands(uint32 swapchainImageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	// get rendering resource from pipeline
	DrawSyncObjects syncObjects = _drawSyncObjects[_currentFrameIndex];

	auto commandBuffer = *(GCommandService->GetCommandBuffer(_currentFrameIndex));
	MK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	// 4. prepare render pass begin info
	auto swapchainExtent = _mkSwapchain.GetSwapchainExtent(); // store swapchain extent for common usage.

	const auto clearColor = glm::vec4(0.01f, 0.01f, 0.01f, 1.f); // settings for VK_ATTACHMENT_LOAD_OP_CLEAR in color attachment
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0] = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]} };   // clear values for color
	clearValues[1] = { 1.0f, 0 };                                                        // clear value for depth and stencil attachment

	if (_mkDevice.enableDynamicRendering)
	{
		auto vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(_mkInstance.GetVkInstance(), "vkCmdBeginRenderingKHR");
		auto vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(_mkInstance.GetVkInstance(), "vkCmdEndRenderingKHR");
		if (!vkCmdBeginRenderingKHR || !vkCmdEndRenderingKHR)
		{
			throw std::runtime_error("Unable to dynamically load vkCmdBeginRenderingKHR and vkCmdEndRenderingKHR");
		}

		// ----------- offscreen rendering ------------
		VkImageSubresourceRange colorRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
		VkImageSubresourceRange depthRange = colorRange;
		depthRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		VkRenderingAttachmentInfoKHR colorAttachmentInfo = mk::vkinfo::GetRenderingAttachmentInfoKHR();
		colorAttachmentInfo.imageView   = _vkOffscreenColorImageView;
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		colorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
		colorAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue = clearValues[0];

		VkRenderingAttachmentInfoKHR depthAttachmentInfo = mk::vkinfo::GetRenderingAttachmentInfoKHR();
		depthAttachmentInfo.imageView   = _vkOffscreenDepthImageView;
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
		depthAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentInfo.clearValue  = clearValues[1];

		auto renderArea = VkRect2D{ VkOffset2D{}, swapchainExtent };
		auto renderInfo = mk::vkinfo::GetRenderingInfoKHR(renderArea, 1, &colorAttachmentInfo);
		renderInfo.layerCount = 1;
		renderInfo.pDepthAttachment = &depthAttachmentInfo;
		if (!IsDepthOnlyFormat(_vkOffscreenDepthFormat))
		{
			renderInfo.pStencilAttachment = &depthAttachmentInfo; // if the depth format includes stencil, then use it as stencil attachment
		}

		vkCmdBeginRenderingKHR(commandBuffer, &renderInfo);
		Rasterize(commandBuffer, swapchainExtent);
		vkCmdEndRenderingKHR(commandBuffer);

		// ----------- post pipeline rendering ------------
		mk::vk::TransitionImageLayout(
			commandBuffer,
			_mkSwapchain.GetSwapchainImage(swapchainImageIndex),
			_mkSwapchain.GetSwapchainImageFormat(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			colorRange
		);
		mk::vk::TransitionImageLayout(
			commandBuffer,
			_mkSwapchain.GetDepthImage(),
			_mkSwapchain.GetDepthFormat(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			depthRange
		);

		VkRenderingAttachmentInfo swapchainColorAttachmentInfo = mk::vkinfo::GetRenderingAttachmentInfoKHR();
		swapchainColorAttachmentInfo.imageView   = _mkSwapchain.GetSwapchainImageView(swapchainImageIndex);	
		swapchainColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		swapchainColorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
		swapchainColorAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
		swapchainColorAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
		swapchainColorAttachmentInfo.clearValue  = clearValues[0];

		VkRenderingAttachmentInfoKHR swapchainDepthAttachmentInfo = mk::vkinfo::GetRenderingAttachmentInfoKHR();
		swapchainDepthAttachmentInfo.imageView   = _mkSwapchain.GetDepthImageView();
		swapchainDepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		swapchainDepthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
		swapchainDepthAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
		swapchainDepthAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
		swapchainDepthAttachmentInfo.clearValue  = clearValues[1];

		auto postRenderArea = VkRect2D{ VkOffset2D{}, swapchainExtent };
		auto postRenderInfo = mk::vkinfo::GetRenderingInfoKHR(postRenderArea, 1, &swapchainColorAttachmentInfo);
		postRenderInfo.layerCount = 1;
		postRenderInfo.pDepthAttachment = &swapchainDepthAttachmentInfo;
		if (!IsDepthOnlyFormat(_vkOffscreenDepthFormat))
		{
			postRenderInfo.pStencilAttachment = &swapchainDepthAttachmentInfo; // if the depth format includes stencil, then use it as stencil attachment
		}

		// begin post rendering
		vkCmdBeginRenderingKHR(commandBuffer, &postRenderInfo);
		// draw post process
		DrawPostProcess(commandBuffer, swapchainExtent);
		// end post rendering
		vkCmdEndRenderingKHR(commandBuffer);

		mk::vk::TransitionImageLayout(
			commandBuffer,
			_mkSwapchain.GetSwapchainImage(swapchainImageIndex),
			_mkSwapchain.GetSwapchainImageFormat(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			colorRange
		);
	}
	else
	{
		/**
		* DEPRECATED (rendering with render pass and frame buffer)
		*/
		// 1. begin offscreen render pass
		VkRenderPassBeginInfo offRenderBeginInfo{};
		offRenderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		offRenderBeginInfo.renderPass = _vkOffscreenRednerPass;
		offRenderBeginInfo.framebuffer = _vkOffscreenFramebuffer;
		offRenderBeginInfo.renderArea.offset = { 0, 0 };
		offRenderBeginInfo.renderArea.extent = swapchainExtent;
		offRenderBeginInfo.clearValueCount = static_cast<uint32>(clearValues.size());
		offRenderBeginInfo.pClearValues = clearValues.data();

		// begin offscreen render pass
		vkCmdBeginRenderPass(commandBuffer, &offRenderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// rasterize
		Rasterize(commandBuffer, swapchainExtent);

		vkCmdEndRenderPass(commandBuffer);

		// 2. begin post pipeline render pass
		VkRenderPassBeginInfo postRenderBeginInfo{};
		postRenderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		postRenderBeginInfo.renderPass = _vkRenderPass;
		postRenderBeginInfo.framebuffer = _vkFramebuffers[swapchainImageIndex];
		postRenderBeginInfo.renderArea.offset = { 0, 0 };
		postRenderBeginInfo.renderArea.extent = swapchainExtent;
		postRenderBeginInfo.clearValueCount = static_cast<uint32>(clearValues.size());
		postRenderBeginInfo.pClearValues = clearValues.data();

		// begin post render pass
		vkCmdBeginRenderPass(commandBuffer, &postRenderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		// draw post process
		DrawPostProcess(commandBuffer, swapchainExtent);
		// end post render pass
		vkCmdEndRenderPass(commandBuffer);
	}
	


	MK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void Renderer::DrawFrame()
{
	// update every states
	Update();

	// 1. wait for the previous frame to be finished
	DrawSyncObjects syncObject = _drawSyncObjects[_currentFrameIndex];
	vkWaitForFences(_mkDevice.GetDevice(), 1, &syncObject.inFlightFence, VK_TRUE, UINT64_MAX);

	// 2. get available image from swapchain
	uint32 imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		_mkDevice.GetDevice(), 
		_mkSwapchain.GetSwapchain(), 
		UINT64_MAX, 
		syncObject.imageAvailableSema,
		VK_NULL_HANDLE, 
		&imageIndex
	);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) // 2.1 error by out of date -> recreate swapchain
	{
		OnResizeWindow();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) // 2.2 error by another reason
	{
		MK_THROW("Failed to acquire swap chain image!.")
	}

	// 3. reset fence if and only if application is submitting commands
	vkResetFences(_mkDevice.GetDevice(), 1, &syncObject.inFlightFence); // make current fence unsignaled

	// 4. reset frame buffer command buffer
	GCommandService->ResetCommandBuffer(_currentFrameIndex);

	// 5. record frame buffer commands (offscreen rendering -> tone mapper -> UI)
	RecordFrameBufferCommands(imageIndex);

	// 6. copy rendering resources and submit recorded command buffer to graphics queue
	VkSemaphore          waitSemaphores[]   = { syncObject.imageAvailableSema };
	VkPipelineStageFlags waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore          signalSemaphores[] = { syncObject.renderFinishedSema };

	GCommandService->SubmitCommandBufferToQueue(
		_currentFrameIndex,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		_mkDevice.GetGraphicsQueue(),
		syncObject.inFlightFence
	);

	// 7. present image to swapchain
	VkPresentInfoKHR presentInfo{};
	VkSwapchainKHR swapChains[] = { _mkSwapchain.GetSwapchain() };

	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = signalSemaphores; // use same signal semaphore to wait on command buffer to finish execution
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
		DrawFrame();
	}

	_mkDevice.WaitUntilDeviceIdle();
}
