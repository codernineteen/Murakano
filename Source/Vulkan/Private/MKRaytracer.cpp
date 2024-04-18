#include "MKRaytracer.h"

// TODO
// 1. Uniform buffer initialization
// 2. destroy resources before destructing MKRaytracer instance
// 3. modify TraceRay logic
// 4. check shader binding table creation
// 5. check ray tracing pipline creation
// 6. add build command buffer

MKRaytracer::~MKRaytracer()
{
	// destroy descriptor set layout
	vkDestroyDescriptorSetLayout(_mkDevicePtr->GetDevice(), _vkRayTracingDescriptorSetLayout, nullptr);


	DestroyAccelerationStructureKHR(_blas);
	DestroyAccelerationStructureKHR(_tlas);

	// destroy ray tracing pipeline
	vkDestroyPipeline(_mkDevicePtr->GetDevice(), _vkRayTracingPipeline, nullptr);
	// destroy ray tracing pipeline layout
	vkDestroyPipelineLayout(_mkDevicePtr->GetDevice(), _vkRayTracingPipelineLayout, nullptr);
	// destroy shader binding table buffer
	vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), _sbtBuffer.buffer, _sbtBuffer.allocation);

#ifndef NDEBUG
	MK_LOG("ray tracer's descriptor set layout destroyed");
	MK_LOG("bottom-level and top-level acceleration structures destroyed");
	MK_LOG("ray tracing pipeline and its layout destroyed");
	MK_LOG("shader binding table buffer destroyed");
#endif
}

void MKRaytracer::LoadVkRaytracingExtension()
{
#ifdef VK_KHR_acceleration_structure
	vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkGetAccelerationStructureBuildSizesKHR");
	vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkGetAccelerationStructureDeviceAddressKHR");
	vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkCreateAccelerationStructureKHR");
	vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkCmdBuildAccelerationStructuresKHR");
	vkCmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkCmdWriteAccelerationStructuresPropertiesKHR");
	vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkDestroyAccelerationStructureKHR");
	vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkCreateRayTracingPipelinesKHR");
	vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkGetRayTracingShaderGroupHandlesKHR");
	vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkCmdTraceRaysKHR");
#else
	MK_THROW("VK_KHR_accleration_structure is not enabled.");
#endif
}

void MKRaytracer::CreateStorageImage(VkExtent2D extent) 
{
	_storageImage.width = extent.width;
	_storageImage.height = extent.height;

	VkImageCreateInfo imageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent = { extent.width , extent.height, 1 }; // width, height, depth
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateImage(_mkDevicePtr->GetDevice(), &imageCreateInfo, nullptr, &_storageImage.image);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_mkDevicePtr->GetDevice(), _storageImage.image, &memRequirements); // populate memory requirements
	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	MK_CHECK(vkAllocateMemory(_mkDevicePtr->GetDevice(), &allocInfo, nullptr, &_storageImage.memory));
	MK_CHECK(vkBindImageMemory(_mkDevicePtr->GetDevice(), _storageImage.image, _storageImage.memory, 0));

	VkImageViewCreateInfo colorImageView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;
	colorImageView.image = _storageImage.image;
	MK_CHECK(vkCreateImageView(_mkDevicePtr->GetDevice(), &colorImageView, nullptr, &_storageImage.imageView));

	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); // transient command pool for one-time command buffer

	VkCommandBuffer commandBuffer;
	GCommandService->BeginSingleTimeCommands(commandBuffer, cmdPool);

	util::TransitionImageLayoutVerbose(
		commandBuffer,
		_storageImage.image,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkAccessFlags(), VkAccessFlags(),
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	);

	GCommandService->EndSingleTimeCommands(commandBuffer, cmdPool);	
}

ScratchBuffer MKRaytracer::CreateScratchBuffer(VkDeviceSize size)
{
	ScratchBuffer scratch_buffer{};

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	MK_CHECK(vkCreateBuffer(_mkDevicePtr->GetDevice(), &bufferCreateInfo, nullptr, &scratch_buffer.handle));

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(_mkDevicePtr->GetDevice(), scratch_buffer.handle, &memoryRequirements);

	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = &memoryAllocateFlagsInfo;
	memory_allocate_info.allocationSize = memoryRequirements.size;
	memory_allocate_info.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	MK_CHECK(vkAllocateMemory(_mkDevicePtr->GetDevice(), &memory_allocate_info, nullptr, &scratch_buffer.memory));
	MK_CHECK(vkBindBufferMemory(_mkDevicePtr->GetDevice(), scratch_buffer.handle, scratch_buffer.memory, 0));

	scratch_buffer.deviceAddress = _mkDevicePtr->GetBufferDeviceAddress(scratch_buffer.handle);

	return scratch_buffer;
}

void MKRaytracer::DestroyScratchBuffer(ScratchBuffer& scratch_buffer)
{
	if (scratch_buffer.memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(_mkDevicePtr->GetDevice(), scratch_buffer.memory, nullptr);
	}
	if (scratch_buffer.handle != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(_mkDevicePtr->GetDevice(), scratch_buffer.handle, nullptr);
	}
}

// create buffer allocated
VkBufferAllocated MKRaytracer::CreateBufferAllocated(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryAllocateFlags memoryFlags, void* data)
{
	VkBufferAllocated stagingBuffer = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_AUTO, // auto-detect memory type
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT// must to be set because of VMA_MEMORY_USAGE_AUTO
	);

	memcpy(stagingBuffer.allocationInfo.pMappedData, data, size);

	VkBufferAllocated bufferAllocated = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
		size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
		memoryUsage,
		VMA_ALLOCATION_CREATE_MAPPED_BIT | memoryFlags
	);

	GCommandService->ExecuteSingleTimeCommands([&](VkCommandBuffer commandBuffer) { // getting reference parameter outer-scope
		VkBufferCopy copyRegion{};
		copyRegion.size = (VkDeviceSize)size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, _indexBuffer.buffer, 1, &copyRegion);
	});

	vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);

	return bufferAllocated;
}

void MKRaytracer::CreateBLAS()
{
	// Setup vertices and indices for a single triangle
	struct SimpleVertex
	{
		float pos[3];
	};
	std::vector<SimpleVertex> vertices = {
		{{1.0f, 1.0f, 0.0f}},
		{{-1.0f, 1.0f, 0.0f}},
		{{0.0f, -1.0f, 0.0f}} 
	};
	std::vector<uint32> indices = { 0, 1, 2 };

	auto vertex_buffer_size = vertices.size() * sizeof(Vertex);
	auto index_buffer_size = indices.size() * sizeof(uint32_t);

	// Create buffers for the bottom level geometry
	// For the sake of simplicity we won't stage the vertex data to the GPU memory

	// Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
	const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;


	_vertexBuffer = CreateBufferAllocated(vertex_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU, 0, (void*)vertices.data());
	_indexBuffer = CreateBufferAllocated(index_buffer_size, buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU, 0, (void*)indices.data());

	// Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
	VkTransformMatrixKHR transform_matrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f 
	};
	VkBufferAllocated transformBuffer = CreateBufferAllocated(sizeof(transform_matrix), buffer_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU, 0, (void*)&transform_matrix);

	VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
	VkDeviceOrHostAddressConstKHR index_data_device_address{};
	VkDeviceOrHostAddressConstKHR transform_matrix_device_address{};

	vertex_data_device_address.deviceAddress = _mkDevicePtr->GetBufferDeviceAddress(_vertexBuffer.buffer);
	index_data_device_address.deviceAddress = _mkDevicePtr->GetBufferDeviceAddress(_indexBuffer.buffer);
	transform_matrix_device_address.deviceAddress = _mkDevicePtr->GetBufferDeviceAddress(transformBuffer.buffer);

	// The bottom level acceleration structure contains one set of triangles as the input geometry
	VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
	acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	acceleration_structure_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	acceleration_structure_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	acceleration_structure_geometry.geometry.triangles.vertexData = vertex_data_device_address;
	acceleration_structure_geometry.geometry.triangles.maxVertex = 3;
	acceleration_structure_geometry.geometry.triangles.vertexStride = sizeof(Vertex);
	acceleration_structure_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	acceleration_structure_geometry.geometry.triangles.indexData = index_data_device_address;
	acceleration_structure_geometry.geometry.triangles.transformData = transform_matrix_device_address;

	// Get the size requirements for buffers involved in the acceleration structure build process
	VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
	acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_structure_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_structure_build_geometry_info.geometryCount = 1;
	acceleration_structure_build_geometry_info.pGeometries = &acceleration_structure_geometry;

	const uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
	acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		_mkDevicePtr->GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&acceleration_structure_build_geometry_info,
		&primitive_count,
		&acceleration_structure_build_sizes_info);

	// Create a buffer to hold the acceleration structure
	_blas.bufferAllocated = CreateBufferAllocated(
		acceleration_structure_build_sizes_info.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0,
		nullptr
	);

	// Create the acceleration structure
	VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
	acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_create_info.buffer = _blas.bufferAllocated.buffer;
	acceleration_structure_create_info.size = acceleration_structure_build_sizes_info.accelerationStructureSize;
	acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(_mkDevicePtr->GetDevice(), &acceleration_structure_create_info, nullptr, &_blas.handle);

	// The actual build process starts here

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	ScratchBuffer scratch_buffer = CreateScratchBuffer(acceleration_structure_build_sizes_info.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
	acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	acceleration_build_geometry_info.dstAccelerationStructure = _blas.handle;
	acceleration_build_geometry_info.geometryCount = 1;
	acceleration_build_geometry_info.pGeometries = &acceleration_structure_geometry;
	acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
	acceleration_structure_build_range_info.primitiveCount = 1;
	acceleration_structure_build_range_info.primitiveOffset = 0;
	acceleration_structure_build_range_info.firstVertex = 0;
	acceleration_structure_build_range_info.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = { &acceleration_structure_build_range_info };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); // transient command pool for one-time command buffer

	VkCommandBuffer commandBuffer;
	GCommandService->BeginSingleTimeCommands(commandBuffer, cmdPool);

	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&acceleration_build_geometry_info,
		acceleration_build_structure_range_infos.data()
	);
	GCommandService->EndSingleTimeCommands(commandBuffer, cmdPool);

	DestroyScratchBuffer(scratch_buffer);

	// Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
	acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	acceleration_device_address_info.accelerationStructure = _blas.handle;
	_blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(_mkDevicePtr->GetDevice(), &acceleration_device_address_info);
}

void MKRaytracer::CreateTLAS()
{
	VkTransformMatrixKHR transform_matrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

	VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
	acceleration_structure_instance.transform = transform_matrix;
	acceleration_structure_instance.instanceCustomIndex = 0;
	acceleration_structure_instance.mask = 0xFF;
	acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
	acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	acceleration_structure_instance.accelerationStructureReference = _blas.deviceAddress;

	VkBufferAllocated instances_buffer = CreateBufferAllocated(
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		0,
		&acceleration_structure_instance	
	);

	VkDeviceOrHostAddressConstKHR instance_data_device_address{};
	instance_data_device_address.deviceAddress = _mkDevicePtr->GetBufferDeviceAddress(instances_buffer.buffer);

	// The top level acceleration structure contains (bottom level) instance as the input geometry
	VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
	acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	acceleration_structure_geometry.geometry.instances.data = instance_data_device_address;

	// Get the size requirements for buffers involved in the acceleration structure build process
	VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
	acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_structure_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_structure_build_geometry_info.geometryCount = 1;
	acceleration_structure_build_geometry_info.pGeometries = &acceleration_structure_geometry;

	const uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
	acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		_mkDevicePtr->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&acceleration_structure_build_geometry_info,
		&primitive_count,
		&acceleration_structure_build_sizes_info);

	// Create a buffer to hold the acceleration structure
	_tlas.bufferAllocated = CreateBufferAllocated(
		acceleration_structure_build_sizes_info.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0,
		nullptr
	);

	// Create the acceleration structure
	VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
	acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_structure_create_info.buffer = _tlas.bufferAllocated.buffer;
	acceleration_structure_create_info.size = acceleration_structure_build_sizes_info.accelerationStructureSize;
	acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	vkCreateAccelerationStructureKHR(_mkDevicePtr->GetDevice(), &acceleration_structure_create_info, nullptr, &_tlas.handle);

	// The actual build process starts here

	// Create a scratch buffer as a temporary storage for the acceleration structure build
	ScratchBuffer scratch_buffer = CreateScratchBuffer(acceleration_structure_build_sizes_info.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
	acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	acceleration_build_geometry_info.dstAccelerationStructure = _tlas.handle;
	acceleration_build_geometry_info.geometryCount = 1;
	acceleration_build_geometry_info.pGeometries = &acceleration_structure_geometry;
	acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
	acceleration_structure_build_range_info.primitiveCount = 1;
	acceleration_structure_build_range_info.primitiveOffset = 0;
	acceleration_structure_build_range_info.firstVertex = 0;
	acceleration_structure_build_range_info.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = { &acceleration_structure_build_range_info };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); // transient command pool for one-time command buffer

	VkCommandBuffer commandBuffer;
	GCommandService->BeginSingleTimeCommands(commandBuffer, cmdPool);

	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&acceleration_build_geometry_info,
		acceleration_build_structure_range_infos.data());
	
	GCommandService->EndSingleTimeCommands(commandBuffer, cmdPool);

	DestroyScratchBuffer(scratch_buffer);

	// Get the top acceleration structure's handle, which will be used to setup it's descriptor
	VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
	acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	acceleration_device_address_info.accelerationStructure = _tlas.handle;
	_tlas.deviceAddress =
		vkGetAccelerationStructureDeviceAddressKHR(_mkDevicePtr->GetDevice(), &acceleration_device_address_info);
}

/**
* ------------- Ray tracing desscriptor set ---------------
*/
void MKRaytracer::InitializeRayTracingDescriptorSet(VkStorageImage& storageImage) 
{
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,               // binding type - storage image
		VK_SHADER_STAGE_RAYGEN_BIT_KHR,                 // shader stage - ray generation
		VkRtxDescriptorBinding::UNIFORM,              // binding point of output image 
		1                                               // number of descriptors
	);
	GDescriptorManager->AddDescriptorSetLayoutBinding( 
		VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,  // binding type - acceleration structure
		VK_SHADER_STAGE_RAYGEN_BIT_KHR,                 // shader stage - ray generation
		VkRtxDescriptorBinding::TLAS,                   // binding point of TLAS
		1                                               // number of descriptors
	);
	GDescriptorManager->AddDescriptorSetLayoutBinding(
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,               // binding type - storage image
		VK_SHADER_STAGE_RAYGEN_BIT_KHR,                 // shader stage - ray generation
		VkRtxDescriptorBinding::OUT_IMAGE,              // binding point of output image 
		1                                               // number of descriptors
	);
	

	// create descriptor set layout with above rtx-related bindings
	GDescriptorManager->CreateDescriptorSetLayout(_vkRayTracingDescriptorSetLayout);
	// allocate descriptor set 
	std::vector<VkDescriptorSet> _vkRayTracingDescriptorSets = { _vkRayTracingDescriptorSet };
	GDescriptorManager->AllocateDescriptorSet(_vkRayTracingDescriptorSets, _vkRayTracingDescriptorSetLayout);

	GDescriptorManager->WriteBufferToDescriptorSet(
		_ubo.buffer,               // uniform buffer
		0,                                         // offset
		sizeof(UniformBuffer),               // range
		VkRtxDescriptorBinding::UNIFORM,                                         // binding point
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER          // descriptor type
	);
	GDescriptorManager->WriteAccelerationStructureToDescriptorSet(&_tlas.handle, VkRtxDescriptorBinding::TLAS);
	GDescriptorManager->WriteImageToDescriptorSet(storageImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VkRtxDescriptorBinding::OUT_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	// update and flush waiting writes
	GDescriptorManager->UpdateDescriptorSet(_vkRayTracingDescriptorSet);
}

void MKRaytracer::UpdateDescriptorImageWrite()
{
	GDescriptorManager->WriteImageToDescriptorSet(_storageImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VkRtxDescriptorBinding::OUT_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	GDescriptorManager->UpdateDescriptorSet(_vkRayTracingDescriptorSet);
}


/**
* ------------- Ray tracing pipeline & shaders ---------------
*/

void MKRaytracer::CreateRayTracingPipeline()
{
		// load ray tracing shaders
	auto raygenShader = util::ReadFile("../../../shaders/output/spir-v/raygenShader.spv");
	auto missShader = util::ReadFile("../../../shaders/output/spir-v/raymissShader.spv");
	auto closestHitShader = util::ReadFile("../../../shaders/output/spir-v/rayclosestShader.spv");

	// create shader modules
	VkShaderModule raygenShaderModule = util::CreateShaderModule(_mkDevicePtr->GetDevice(), raygenShader);
	VkShaderModule missShaderModule = util::CreateShaderModule(_mkDevicePtr->GetDevice(), missShader);
	VkShaderModule closestHitShaderModule = util::CreateShaderModule(_mkDevicePtr->GetDevice(), closestHitShader);

	// create shader stages
	VkPipelineShaderStageCreateInfo raygenShaderStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	raygenShaderStageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	raygenShaderStageInfo.module = raygenShaderModule;
	raygenShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo missShaderStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	missShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	missShaderStageInfo.module = missShaderModule;
	missShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo closestHitShaderStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	closestHitShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	closestHitShaderStageInfo.module = closestHitShaderModule;
	closestHitShaderStageInfo.pName = "main";

	// create shader groups
	VkRayTracingShaderGroupCreateInfoKHR group{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
	group.anyHitShader = VK_SHADER_UNUSED_KHR;
	group.intersectionShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = VK_SHADER_UNUSED_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;

	// create raygen shader group
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = VkRtxShaderStages::RAYGEN;
	_vkShaderGroupsInfo.push_back(group);

	// create miss shader group
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group.generalShader = VkRtxShaderStages::MISS;
	_vkShaderGroupsInfo.push_back(group);

	// create closest hit shader group
	group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	group.generalShader = VK_SHADER_UNUSED_KHR;
	group.closestHitShader = VkRtxShaderStages::CLOSEST_HIT;
	_vkShaderGroupsInfo.push_back(group);

	
	/**
	* pipeline creation starts
	*/

	// populate descriptor set layouts into a vector
	// - external descriptor set layout holds uniform buffer and texture samplers binding
	// - ray tracing descriptor set layout holds acceleration structure and storage image binding
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ _vkRayTracingDescriptorSetLayout }; 

	// create ray tracing pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	// specify descriptor set layouts information into ray tracing pipeline layout
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	vkCreatePipelineLayout(_mkDevicePtr->GetDevice(), &pipelineLayoutInfo, nullptr, &_vkRayTracingPipelineLayout);

	// populate shader stages
	std::array<VkPipelineShaderStageCreateInfo, VkRtxShaderStages::STAGE_COUNT> shaderStages{ raygenShaderStageInfo, missShaderStageInfo, closestHitShaderStageInfo };

	// create ray tracing pipeline
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	rayTracingPipelineInfo.stageCount = static_cast<uint32>(shaderStages.size());        // shader stages info
	rayTracingPipelineInfo.pStages = shaderStages.data();
	rayTracingPipelineInfo.groupCount = static_cast<uint32>(_vkShaderGroupsInfo.size()); // shader groups info
	rayTracingPipelineInfo.pGroups = _vkShaderGroupsInfo.data();
	rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 1;                             // recursion depth (small number of depth is desirable for performance.)
	rayTracingPipelineInfo.layout = _vkRayTracingPipelineLayout;                         // pipeline layout

	vkCreateRayTracingPipelinesKHR(_mkDevicePtr->GetDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &_vkRayTracingPipeline);

	// destroy shader modules
	for (auto& stage : shaderStages) 
		vkDestroyShaderModule(_mkDevicePtr->GetDevice(), stage.module, nullptr);
}

void MKRaytracer::CreateShaderBindingTable()
{
	auto rayTrackingPipelineProperties = GetRayTracingPipelineProperties();

	// shader counts
	const uint32 missCount{ 1 };
	const uint32 hitCount{ 1 };

	// handle size and count
	const uint32 handleSize = rayTrackingPipelineProperties.shaderGroupHandleSize;
	const uint32 handleCount = 1 + missCount + hitCount;

	const uint32 handleSizeAligned = GetAlignedSize(handleSize, rayTrackingPipelineProperties.shaderGroupHandleAlignment);

	/**
	* Specify regions for ray generation, miss, and hit shaders
	* 	- stride : size of the handle aligned to `shaderGroupHandleAlignment`
	* 	- size : size of the handle aligned to `shaderGroupHandleAlignment`
	*/
	_raygenRegion.stride = GetAlignedSize(handleSizeAligned, rayTrackingPipelineProperties.shaderGroupBaseAlignment);
	_raygenRegion.size = _raygenRegion.stride;

	_raymissRegion.stride = handleSizeAligned;
	_raymissRegion.size = GetAlignedSize(handleSizeAligned * missCount, rayTrackingPipelineProperties.shaderGroupBaseAlignment);

	_rayhitRegion.stride = handleSizeAligned;
	_rayhitRegion.size = GetAlignedSize(handleSizeAligned * hitCount, rayTrackingPipelineProperties.shaderGroupBaseAlignment);

	// create shader handles
	uint32 dataSize = handleCount * handleSize;
	std::vector<uint8> shaderHandles(dataSize);
	MK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(_mkDevicePtr->GetDevice(), _vkRayTracingPipeline, 0, handleCount, dataSize, shaderHandles.data()));

	// create shader binding table buffer that hold 'handle data'
	VkDeviceSize sbtSize = _raygenRegion.size + _raymissRegion.size + _rayhitRegion.size + _callableRegion.size;
	const VkBufferUsageFlags sbtUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
	_sbtBuffer = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
		sbtSize,
		sbtUsageFlags,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // host visible, host coherent
		0 // no memory allocation flags
	);

	// store device address offset for each shader region.
	auto sbtBufferDeviceAddress = _mkDevicePtr->GetBufferDeviceAddress(_sbtBuffer.buffer);
	_raygenRegion.deviceAddress = sbtBufferDeviceAddress;
	_raymissRegion.deviceAddress = sbtBufferDeviceAddress + _raygenRegion.size;
	_rayhitRegion.deviceAddress = sbtBufferDeviceAddress + _raygenRegion.size + _raymissRegion.size;

	void* voidData;
	vmaMapMemory(_mkDevicePtr->GetVmaAllocator(), _sbtBuffer.allocation, &voidData);
	uint8* pSbtBuffer = reinterpret_cast<uint8*>(voidData);
	uint8* data = nullptr;
	uint32 handleIndex = 0;

	// helper function to get handle 
	auto GetHandle = [&](uint32 idx) { return shaderHandles.data() + idx * handleSize; };

	// copy ray gen handles to the data
	data = pSbtBuffer;
	memcpy(data, GetHandle(handleIndex++), handleSize);
	
	// copy miss handles to the data
	data = pSbtBuffer + _raygenRegion.size;
	for (uint32 i = 0; i < missCount; i++)
	{
		memcpy(data, GetHandle(handleIndex++), handleSize);
		data += _raymissRegion.stride;
	}

	// copy hit handles to the data
	data = pSbtBuffer + _raygenRegion.size + _raymissRegion.size;
	for (uint32 i = 0; i < hitCount; i++)
	{
		memcpy(data, GetHandle(handleIndex++), handleSize);
		data += _rayhitRegion.stride;
	}

	// unmap memory
	vmaUnmapMemory(_mkDevicePtr->GetVmaAllocator(), _sbtBuffer.allocation);
	pSbtBuffer = nullptr;
}

void MKRaytracer::TraceRay(VkCommandBuffer commandBuffer, const glm::vec4 clearColor, VkDescriptorSet externDescSet, VkPushConstantRaster rasterConstant, VkExtent2D extent)
{
	/**
	* populate descriptor sets
	* - set 0 : ray tracing descriptor set
	* - set 1 : external descriptor set
	*/
	std::vector<VkDescriptorSet> descriptorSets = { _vkRayTracingDescriptorSet };
	// bind ray tracing pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _vkRayTracingPipeline);
	// bind descriptor sets
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
		_vkRayTracingPipelineLayout, 
		0, 
		static_cast<uint32>(descriptorSets.size()), 
		descriptorSets.data(), 
		0, 
		nullptr
	);

	// record ray trace command
	vkCmdTraceRaysKHR(
		commandBuffer,
		&_raygenRegion,
		&_raymissRegion,
		&_rayhitRegion,
		&_callableRegion,
		extent.width,
		extent.height,
		1
	);
}


/**
* --------------------- Helpers ---------------------------
*/

bool MKRaytracer::HasFlag(VkFlags item, VkFlags flag)
{
	return (item & flag) == flag;
}


// find memory type
uint32 MKRaytracer::FindMemoryType(uint32 bits, VkMemoryPropertyFlags properties, VkBool32* memoryTypeFound) {
	VkPhysicalDeviceMemoryProperties deviceMemProperties;
	vkGetPhysicalDeviceMemoryProperties(_mkDevicePtr->GetPhysicalDevice(), &deviceMemProperties);

	for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((deviceMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memoryTypeFound)
				{
					*memoryTypeFound = true;
				}
				return i;
			}
		}
		bits >>= 1;
	}

	if (memoryTypeFound)
	{
		*memoryTypeFound = false;
		return 0;
	}
	else
	{
		MK_THROW("Could not find a matching memory type");
	}
}


// Convert glm::mat4 to VkTransformMatrixKHR
VkTransformMatrixKHR MKRaytracer::ConvertGLMToVkMat4(const glm::mat4& matrix)
{
	/**
	* VkTransformMatrixKHR - row major matrix
	* glm::mat4            - column major matrix
	*/
	glm::mat4 transposed = glm::transpose(matrix);
	VkTransformMatrixKHR outMatrix;
	memcpy(&outMatrix, &transposed, sizeof(VkTransformMatrixKHR));
	return outMatrix;
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR MKRaytracer::GetRayTracingPipelineProperties()
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	VkPhysicalDeviceProperties2 deviceProperties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

	deviceProperties2.pNext = &rayTracingPipelineProperties;
	
	vkGetPhysicalDeviceProperties2(_mkDevicePtr->GetPhysicalDevice(), &deviceProperties2);
	return rayTracingPipelineProperties;
}


void MKRaytracer::DestroyAccelerationStructureKHR(AcceleraationStructure& accelStruct)
{
	vkDestroyAccelerationStructureKHR(_mkDevicePtr->GetDevice(), accelStruct.handle, nullptr);
	vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), accelStruct.bufferAllocated.buffer, accelStruct.bufferAllocated.allocation);
}