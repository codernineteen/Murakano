#include "MKRaytracer.h"

MKRaytracer::~MKRaytracer()
{
	// destroy descriptor set layout
	vkDestroyDescriptorSetLayout(_mkDevicePtr->GetDevice(), _vkRayTracingDescriptorSetLayout, nullptr);

	for (auto& blas : _blases)
	{
		DestroyAccelerationStructureKHR(blas);
	}
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
#else
	MK_THROW("VK_KHR_accleration_structure is not enabled.");
#endif
}

void MKRaytracer::BuildRayTracer(MKDevice* devicePtr, const OBJModel& model, const std::vector<OBJInstance>& instances, VkDeviceAddress vertexAddr, VkDeviceAddress indexAddr)
{
	_mkDevicePtr = devicePtr;
	_graphicsQueueIndex = _mkDevicePtr->FindQueueFamilies(_mkDevicePtr->GetPhysicalDevice()).graphicsFamily.value();
	LoadVkRaytracingExtension();
	// initialize buffer device address
	_vertexDeviceAddress = vertexAddr;
	_indexDeviceAddress = indexAddr;
	// create bottom level acceleration structure
	CreateBLAS(model);
	// create top level acceleration structure
	CreateTLAS(instances);



}

VkBLAS MKRaytracer::ObjectToVkGeometryKHR(const OBJModel& model)
{
	// TODO : move vertex and index buffer resources into model class
	// get raw vetex and index buffer device address
	uint32 maxPrimitiveCount = SafeStaticCast<size_t, uint32>(model.indices.size()) / 3;

	// specify acceleration triangle geometry data
	VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
	triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	// specify vertex information including device address
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexStride = sizeof(Vertex);
	triangles.vertexData.deviceAddress = _vertexDeviceAddress;
	triangles.maxVertex = SafeStaticCast<size_t, uint32>(model.vertices.size())-1;
	// specify index information including device address
	triangles.indexType = VK_INDEX_TYPE_UINT32;
	triangles.indexData.deviceAddress = _indexDeviceAddress;

	// specify acceleration structure geometry
	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.geometry.triangles = triangles;

	// specify acceleration structure build range info
	VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
	buildRangeInfo.firstVertex = 0;
	buildRangeInfo.primitiveCount = maxPrimitiveCount;
	buildRangeInfo.primitiveOffset = 0;
	buildRangeInfo.transformOffset = 0;

	VkBLAS blas;
	blas.geometry.emplace_back(geometry);
	blas.buildRangeInfo.emplace_back(buildRangeInfo);

	return blas;
}

VkDeviceAddress MKRaytracer::GetBLASDeviceAddress(uint32 index)
{
	assert(size_t(index) < _blases.size()); // index should be less tn
	VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	addressInfo.accelerationStructure = _blases[index].handle;
	return vkGetAccelerationStructureDeviceAddressKHR(_mkDevicePtr->GetDevice(), &addressInfo);
}

void MKRaytracer::CreateBLAS(const OBJModel& model)
{
	std::vector<VkBLAS> blases;
	// TODO : single model to multiple models
	blases.reserve(1); // reserve space for single model now.

	auto blas = ObjectToVkGeometryKHR(model);
	blases.emplace_back(blas);
	
	BuildBLAS(blases, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void MKRaytracer::BuildBLAS(const std::vector<VkBLAS>& blaseInputs, VkBuildAccelerationStructureFlagsKHR flags) 
{
	uint32 blasCount = static_cast<uint32>(blaseInputs.size());
	VkDeviceSize totalSize{ 0 };     // size of all allocated BLAS
	uint32_t     compactionsCount{ 0 };// number of BLAS requesting compaction
	VkDeviceSize maxScratchSize{ 0 };  // largest scratch size

	// populate acceleration structure build info for each BLAS.
	std::vector<VkAccelerationStructureKHRInfo> buildAsInfo(blasCount);
	for (uint32 idx = 0; idx < blasCount; idx++)
	{
		// Filling partially the VkAccelerationStructureBuildGeometryInfoKHR for querying the build sizes.
		// Other information will be filled in the createBlas (see #2)
		buildAsInfo[idx].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildAsInfo[idx].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildAsInfo[idx].buildInfo.flags = blaseInputs[idx].flags | flags;
		buildAsInfo[idx].buildInfo.geometryCount = static_cast<uint32_t>(blaseInputs[idx].geometry.size());
		buildAsInfo[idx].buildInfo.pGeometries = blaseInputs[idx].geometry.data();
		buildAsInfo[idx].rangeInfo = blaseInputs[idx].buildRangeInfo.data();

		// Finding sizes to create acceleration structures and scratch
		std::vector<uint32_t> maxPrimCount(blaseInputs[idx].buildRangeInfo.size());
		for (auto tt = 0; tt < blaseInputs[idx].buildRangeInfo.size(); tt++)
			maxPrimCount[tt] = blaseInputs[idx].buildRangeInfo[tt].primitiveCount;  // Number of primitives/triangles

		vkGetAccelerationStructureBuildSizesKHR(
			_mkDevicePtr->GetDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&buildAsInfo[idx].buildInfo,
			maxPrimCount.data(),
			&buildAsInfo[idx].sizeInfo
		);

		// Extra info
		totalSize += buildAsInfo[idx].sizeInfo.accelerationStructureSize;
		// track mamximum scratch size needed to allocate a scratch buffer following the size.
		maxScratchSize = std::max(maxScratchSize, buildAsInfo[idx].sizeInfo.buildScratchSize);
		// check compaction flag
		compactionsCount += HasFlag(buildAsInfo[idx].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}

	auto scratchBuffer = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
		maxScratchSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	VkDeviceAddress scratchAddress = _mkDevicePtr->GetBufferDeviceAddress(scratchBuffer.buffer);


	VkQueryPool queryPool{ VK_NULL_HANDLE };
	if (compactionsCount > 0)  // Is compaction requested?
	{
		assert(compactionsCount == blasCount);  // Don't allow mix of on/off compaction
		VkQueryPoolCreateInfo queryPoolInfo{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		queryPoolInfo.queryCount = blasCount;
		queryPoolInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		vkCreateQueryPool(_mkDevicePtr->GetDevice(), &queryPoolInfo, nullptr, &queryPool);
	}

	std::vector<uint32>  indices;  // Indices of the BLAS to create
	VkDeviceSize         batchSize{ 0 };
	VkDeviceSize         batchLimit{ 256'000'000 };  // 256 MB

	for (uint32 idx = 0; idx < blasCount; idx++)
	{
		indices.push_back(idx);
		batchSize += buildAsInfo[idx].sizeInfo.accelerationStructureSize;

		// if the batch is full(over 256MB) or it is the last element
		if (batchSize >= batchLimit || idx == blasCount - 1)
		{
			VkCommandPool cmdPool;
			GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); // transient command pool for one-time command buffer

			VkCommandBuffer commandBuffer;
			GCommandService->BeginSingleTimeCommands(commandBuffer, cmdPool);

			
			CmdCreateBLAS(commandBuffer, indices, buildAsInfo, scratchAddress, queryPool);
			if (queryPool)
			{
				CmdCreateCompactBLAS(commandBuffer, indices, buildAsInfo, queryPool);
				// destroy the non-compacted version
				DestroyNonCompactedBLAS(indices, buildAsInfo);
			}
			
			// end command buffer -> submit -> wait -> destroy
			GCommandService->EndSingleTimeCommands(commandBuffer, cmdPool);

			// Reset batch
			batchSize = 0;
			indices.clear();
		}
	}

	for(auto& b : buildAsInfo) 
		_blases.emplace_back(b.accelStruct);

	// cleanup
	vkDestroyQueryPool(_mkDevicePtr->GetDevice(), queryPool, nullptr);
	vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), scratchBuffer.buffer, scratchBuffer.allocation);
}

/**
* ------------- Ray tracing desscriptor set ---------------
*/
void MKRaytracer::InitializeRayTracingDescriptorSet(VkStorageImage& storageImage) 
{
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
	// allocate descriptor set layout
	_vkRayTracingDescriptorSet.resize(1);
	// allocate descriptor set 
	GDescriptorManager->AllocateDescriptorSet(_vkRayTracingDescriptorSet, _vkRayTracingDescriptorSetLayout);

	GDescriptorManager->WriteAccelerationStructureToDescriptorSet(&_tlas.handle, VkRtxDescriptorBinding::TLAS);
	GDescriptorManager->WriteImageToDescriptorSet(storageImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VkRtxDescriptorBinding::OUT_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	// update and flush waiting writes
	GDescriptorManager->UpdateDescriptorSet(_vkRayTracingDescriptorSet[0]);
}

void MKRaytracer::UpdateDescriptorImageWrite(VkStorageImage& storageImage)
{
	GDescriptorManager->WriteImageToDescriptorSet(storageImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VkRtxDescriptorBinding::OUT_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	GDescriptorManager->UpdateDescriptorSet(_vkRayTracingDescriptorSet[0]);
}


/**
* ------------- Ray tracing pipeline & shaders ---------------
*/

void MKRaytracer::CreateRayTracingPipeline(VkDescriptorSetLayout externalDescriptorSetLayout)
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

	// specify the shaders that use ray push constants
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(VkPushConstantRay);

	// populate descriptor set layouts into a vector
	// - external descriptor set layout holds uniform buffer and texture samplers binding
	// - ray tracing descriptor set layout holds acceleration structure and storage image binding
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ _vkRayTracingDescriptorSetLayout, externalDescriptorSetLayout }; 

	// create ray tracing pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	// specify push constant range information into ray tracing pipeline layout
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
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

	// sbt : shader binding table
	const uint32 handleSize = rayTrackingPipelineProperties.shaderGroupHandleSize;
	const uint32 handleAlignment = rayTrackingPipelineProperties.shaderGroupHandleAlignment;
	const uint32 handleSizeAligned = GetAlignedSize(handleSize, handleAlignment);
	const uint32 groupCount = static_cast<uint32>(_vkShaderGroupsInfo.size());
	const uint32 sbtSize = groupCount * handleSizeAligned;

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
	std::vector<uint8> shaderHandles(sbtSize);
	MK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(_mkDevicePtr->GetDevice(), _vkRayTracingPipeline, 0, groupCount, sbtSize, shaderHandles.data()));

	// create shader binding table buffer that hold 'handle data'
	const VkBufferUsageFlags sbtUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
	_sbtBuffer = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
		handleSize,
		sbtUsageFlags,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // host visible, host coherent
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
	);

	// store device address offset for each shader region.
	auto sbtBufferDeviceAddress = _mkDevicePtr->GetBufferDeviceAddress(_sbtBuffer.buffer);
	_raygenRegion.deviceAddress = sbtBufferDeviceAddress;
	_raymissRegion.deviceAddress = sbtBufferDeviceAddress + _raygenRegion.size;
	_rayhitRegion.deviceAddress = sbtBufferDeviceAddress + _raygenRegion.size + _raymissRegion.size;

	uint8* pSbtBuffer = reinterpret_cast<uint8*>(_sbtBuffer.allocationInfo.pMappedData);
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
}


/**
* --------------------- Helpers ---------------------------
*/

bool MKRaytracer::HasFlag(VkFlags item, VkFlags flag)
{
	return (item & flag) == flag;
}

VkAccelKHR MKRaytracer::CreateAccelerationStructureKHR(VkAccelerationStructureCreateInfoKHR& accelCreateInfo)
{
	VkAccelKHR resultAccel{};
	resultAccel.buffer = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
		accelCreateInfo.size,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // shader device address for building TLAS
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	
	// set allocated buffer to buffer field of acceleration structure
	accelCreateInfo.buffer = resultAccel.buffer.buffer;

	// create acceleration structure (memory binding and allocation with a single function call)
	vkCreateAccelerationStructureKHR(_mkDevicePtr->GetDevice(), &accelCreateInfo, nullptr, &resultAccel.handle);

	return resultAccel;
}

void MKRaytracer::CmdCreateBLAS(VkCommandBuffer commandBuffer, std::vector<uint32>& indices, std::vector<VkAccelerationStructureKHRInfo>& buildAsInfo, VkDeviceAddress scratchAddress, VkQueryPool queryPool)
{
	if (queryPool)
		vkResetQueryPool(_mkDevicePtr->GetDevice(), queryPool, 0, SafeStaticCast<size_t, uint32>(indices.size()));
	uint32 queryCounter{ 0 };

	/**
	* 1. Creating the accleration structure
	* 2. Building the acceleration structure
	*/
	for (const auto& idx : indices) 
	{
		// allocate actual buffer and acceleration structure
		VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = buildAsInfo[idx].sizeInfo.accelerationStructureSize;
		buildAsInfo[idx].accelStruct = CreateAccelerationStructureKHR(createInfo);

		// build info
		buildAsInfo[idx].buildInfo.dstAccelerationStructure = buildAsInfo[idx].accelStruct.handle;
		buildAsInfo[idx].buildInfo.scratchData.deviceAddress = scratchAddress;

		// build bottom level acceleration structure
		vkCmdBuildAccelerationStructuresKHR(
			commandBuffer,
			1,
			&buildAsInfo[idx].buildInfo,
			&buildAsInfo[idx].rangeInfo
		);

		VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		vkCmdPipelineBarrier(
			commandBuffer, 
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 
			0, 
			1, &barrier, 
			0, nullptr, 
			0, nullptr
		);

		if (queryPool) 
		{
			// query acceleration structure size if it needs
			vkCmdWriteAccelerationStructuresPropertiesKHR(
				commandBuffer, 
				1, 
				&buildAsInfo[idx].buildInfo.dstAccelerationStructure, 
				VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, // set this flag to get the number of bytes required by a compacted acceleration structure
				queryPool, 
				queryCounter++
			);
		}
	}
}

// TODO : Implement memory compaction later.
void MKRaytracer::CmdCreateCompactBLAS(VkCommandBuffer commandBuffer, std::vector<uint32> indices, std::vector<VkAccelerationStructureKHRInfo>& buildAsInfo, VkQueryPool queryPool)
{
	// 1. get the values from the query
	// 2. create a new acceleration structure with the smaller size
	// 3. copy the previous as to the new allocated one
	// 4. destroy the previous one.
}

// TODO : Implement destroyer later.
void MKRaytracer::DestroyNonCompactedBLAS(std::vector<uint32> indices, std::vector<VkAccelerationStructureKHRInfo>& buildAsInfo)
{
	// destroy previous acceleration structure after copying it to compacted one.
}

void MKRaytracer::CreateTLAS(const std::vector<OBJInstance> instances)
{
	std::vector<VkAccelerationStructureInstanceKHR> tlases;
	tlases.reserve(instances.size());

	for (const auto& inst : instances) 
	{
		VkAccelerationStructureInstanceKHR rayInstance{};
		rayInstance.transform = ConvertGLMToVkMat4(inst.transform);
		rayInstance.instanceCustomIndex = inst.objIndex;
		rayInstance.accelerationStructureReference = GetBLASDeviceAddress(inst.objIndex);
		rayInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		rayInstance.mask = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
		rayInstance.instanceShaderBindingTableRecordOffset = 0;
		tlases.emplace_back(rayInstance);
	}
	BuildTLAS(tlases, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}

void MKRaytracer::BuildTLAS(const std::vector<VkAccelerationStructureInstanceKHR>& tlases, VkBuildAccelerationStructureFlagsKHR flags, bool isUpdated)
{
	assert(_tlas.handle == VK_NULL_HANDLE || isUpdated); // TLAS should be created before updating it.
	uint32 instanceCount = static_cast<uint32>(tlases.size());

	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); // transient command pool for one-time command buffer

	VkCommandBuffer commandBuffer;
	GCommandService->BeginSingleTimeCommands(commandBuffer, cmdPool);

	/**
	* Upload Vulkan instances to the device
	*/
	// create gpu-resident buffer for instance data and record copy command
	VkBufferAllocated instanceBuffer = CreateBufferWithInstanceData(
		commandBuffer,
		_mkDevicePtr->GetVmaAllocator(),
		tlases,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	VkDeviceAddress instanceBufferAddr = _mkDevicePtr->GetBufferDeviceAddress(instanceBuffer.buffer);

	// record barrier command
	VkMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		0,
		1, &barrier,
		0, nullptr,
		0, nullptr
	);

	// scratch buffer is only required while building acceleration structures.
	VkBufferAllocated scratchBuffer;
	CmdCreateTLAS(commandBuffer, instanceCount, instanceBufferAddr, scratchBuffer, flags, isUpdated);

	// end command buffer -> submit -> wait -> destroy
	GCommandService->EndSingleTimeCommands(commandBuffer, cmdPool);

	// destroy buffers after building TLAS
	vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), scratchBuffer.buffer, scratchBuffer.allocation);
	vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), instanceBuffer.buffer, instanceBuffer.allocation);
}

void MKRaytracer::CmdCreateTLAS(
	VkCommandBuffer commandBuffer,
	uint32 instanceCount,
	VkDeviceAddress instanceBufferDeviceAddr,
	VkBufferAllocated& scratchBuffer,
	VkBuildAccelerationStructureFlagsKHR flags,
	bool update
) 
{
	// acceleration geometry instance data
	VkAccelerationStructureGeometryInstancesDataKHR vkAsInstances{};
	vkAsInstances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	vkAsInstances.data.deviceAddress = instanceBufferDeviceAddr;

	// acceleration geometry
	VkAccelerationStructureGeometryKHR vkAsGeometry{};
	vkAsGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	vkAsGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	vkAsGeometry.geometry.instances = vkAsInstances;

	// specify geometry build info
	VkAccelerationStructureBuildGeometryInfoKHR geoBuildInfo{};
	geoBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	geoBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;                                                               // top level acceleration structure
	geoBuildInfo.flags = flags;
	geoBuildInfo.geometryCount = 1;
	geoBuildInfo.pGeometries = &vkAsGeometry;
	geoBuildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;  // depends on update flag
	geoBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;                                                                         // source acceleration structure

	// specify build size info
	VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo{};
	buildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		_mkDevicePtr->GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&geoBuildInfo,
		&instanceCount,
		&buildSizeInfo
	);

	// create initial TLAS
	if (!update)
	{
		VkAccelerationStructureCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		createInfo.size = buildSizeInfo.accelerationStructureSize; // the size taken from the build size info

		_tlas = CreateAccelerationStructureKHR(createInfo);
	}

	// Building actual TLAS
	scratchBuffer = util::CreateBuffer( // allocate scratch memory
		_mkDevicePtr->GetVmaAllocator(),
		buildSizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	VkDeviceAddress scratchAddress = _mkDevicePtr->GetBufferDeviceAddress(scratchBuffer.buffer);

	// update build info 
	geoBuildInfo.srcAccelerationStructure = update ? _tlas.handle : VK_NULL_HANDLE;
	geoBuildInfo.dstAccelerationStructure = _tlas.handle;
	geoBuildInfo.scratchData.deviceAddress = scratchAddress;

	VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{ instanceCount, 0, 0, 0 };
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

	// Build the TLAS
	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &geoBuildInfo, &pBuildOffsetInfo);
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


void MKRaytracer::DestroyAccelerationStructureKHR(VkAccelKHR& accelStruct)
{
	vkDestroyAccelerationStructureKHR(_mkDevicePtr->GetDevice(), accelStruct.handle, nullptr);
	vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), accelStruct.buffer.buffer, accelStruct.buffer.allocation);
}