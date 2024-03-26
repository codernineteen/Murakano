#include "MKRaytracer.h"

MKRaytracer::MKRaytracer(MKDevice& mkDeviceRef, MKGraphicsPipeline& mkPipelineRef)
	:
	_mkDeviceRef(mkDeviceRef),
	_mkPipelineRef(mkPipelineRef)
{
	_graphicsQueueIndex = mkDeviceRef.FindQueueFamilies(mkDeviceRef.GetPhysicalDevice()).graphicsFamily.value();
	LoadVkRaytracingExtension();
}

MKRaytracer::~MKRaytracer()
{
	for (auto& blas : _blases)
	{
		DestroyAccelerationStructureKHR(blas);
	}
	DestroyAccelerationStructureKHR(_tlas);
}

void MKRaytracer::LoadVkRaytracingExtension()
{
#ifdef VK_KHR_acceleration_structure
	vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(_mkDeviceRef.GetDevice(), "vkGetAccelerationStructureBuildSizesKHR");
	vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(_mkDeviceRef.GetDevice(), "vkGetAccelerationStructureDeviceAddressKHR");
	vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(_mkDeviceRef.GetDevice(), "vkCreateAccelerationStructureKHR");
	vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(_mkDeviceRef.GetDevice(), "vkCmdBuildAccelerationStructuresKHR");
	vkCmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetDeviceProcAddr(_mkDeviceRef.GetDevice(), "vkCmdWriteAccelerationStructuresPropertiesKHR");
	vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(_mkDeviceRef.GetDevice(), "vkDestroyAccelerationStructureKHR");
#else
	MK_THROW("VK_KHR_accleration_structure is not enabled.");
#endif
}

void MKRaytracer::BuildRayTracer(const OBJModel& model, const std::vector<OBJInstance>& instances)
{
	// create bottom level acceleration structure
	CreateBLAS(model);
	// create top level acceleration structure
	CreateTLAS(instances);
}

VkBLAS MKRaytracer::ObjectToVkGeometryKHR(const OBJModel& model)
{
	// TODO : move vertex and index buffer resources into model class
	// get raw vetex and index buffer device address
	VkDeviceAddress vertexAddr = _mkDeviceRef.GetBufferDeviceAddress(_mkPipelineRef.GetVertexBuffer());
	VkDeviceAddress indexAddr = _mkDeviceRef.GetBufferDeviceAddress(_mkPipelineRef.GetIndexBuffer());

	uint32 maxPrimitiveCount = SafeStaticCast<size_t, uint32>(model.indices.size()) / 3;

	// specify acceleration triangle geometry data
	VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
	triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	// specify vertex information including device address
	triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangles.vertexStride = sizeof(Vertex);
	triangles.vertexData.deviceAddress = vertexAddr;
	triangles.maxVertex = SafeStaticCast<size_t, uint32>(model.vertices.size())-1;
	// specify index information including device address
	triangles.indexType = VK_INDEX_TYPE_UINT32;
	triangles.indexData.deviceAddress = indexAddr;

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
	return vkGetAccelerationStructureDeviceAddressKHR(_mkDeviceRef.GetDevice(), &addressInfo);
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
			_mkDeviceRef.GetDevice(),
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
		_mkDeviceRef.GetVmaAllocator(),
		maxScratchSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	VkDeviceAddress scratchAddress = _mkDeviceRef.GetBufferDeviceAddress(scratchBuffer.buffer);


	VkQueryPool queryPool{ VK_NULL_HANDLE };
	if (compactionsCount > 0)  // Is compaction requested?
	{
		assert(compactionsCount == blasCount);  // Don't allow mix of on/off compaction
		VkQueryPoolCreateInfo queryPoolInfo{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		queryPoolInfo.queryCount = blasCount;
		queryPoolInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		vkCreateQueryPool(_mkDeviceRef.GetDevice(), &queryPoolInfo, nullptr, &queryPool);
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
			std::queue<VoidLambda> commandQueue;
			commandQueue.push([&](VkCommandBuffer commandBuffer) {
					CmdCreateBLAS(commandBuffer, indices, buildAsInfo, scratchAddress, queryPool);
					if (queryPool)
					{
						CmdCreateCompactBLAS(commandBuffer, indices, buildAsInfo, queryPool);
						// destroy the non-compacted version
						DestroyNonCompactedBLAS(indices, buildAsInfo);
					}
			});
			GCommandService->AsyncExecuteCommands(commandQueue);

			
			// Reset batch
			batchSize = 0;
			indices.clear();
		}
	}

	for(auto& b : buildAsInfo) 
		_blases.emplace_back(b.accelStruct);

	// cleanup
	vkDestroyQueryPool(_mkDeviceRef.GetDevice(), queryPool, nullptr);
	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), scratchBuffer.buffer, scratchBuffer.allocation);
}

bool MKRaytracer::HasFlag(VkFlags item, VkFlags flag)
{
	return (item & flag) == flag;
}

VkAccelKHR MKRaytracer::CreateAccelerationStructureKHR(VkAccelerationStructureCreateInfoKHR& accelCreateInfo)
{
	VkAccelKHR resultAccel{};
	resultAccel.buffer = util::CreateBuffer(
		_mkDeviceRef.GetVmaAllocator(),
		accelCreateInfo.size,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // shader device address for building TLAS
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	
	// set allocated buffer to buffer field of acceleration structure
	accelCreateInfo.buffer = resultAccel.buffer.buffer;

	// create acceleration structure (memory binding and allocation with a single function call)
	vkCreateAccelerationStructureKHR(_mkDeviceRef.GetDevice(), &accelCreateInfo, nullptr, &resultAccel.handle);

	return resultAccel;
}

void MKRaytracer::CmdCreateBLAS(VkCommandBuffer commandBuffer, std::vector<uint32> indices, std::vector<VkAccelerationStructureKHRInfo>& buildAsInfo, VkDeviceAddress scratchAddress, VkQueryPool queryPool)
{
	if (queryPool)
		vkResetQueryPool(_mkDeviceRef.GetDevice(), queryPool, 0, SafeStaticCast<size_t, uint32>(indices.size()));
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
			0,           // dependency flags
			1, &barrier, // memory barrier
			0, nullptr,  // buffer memory barrier
			0, nullptr   // image memory barrier
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
	uint32 instanceCount = SafeStaticCast<size_t, uint32>(tlases.size());

	// ensure that copying the instance data to the buffer is finished before building the TLAS
	GCommandService->ExecuteSingleTimeCommands([&](VkCommandBuffer commandBuffer) {
		/**
		* Upload Vulkan instances to the device
		*/
		// create gpu-resident buffer for instance data and record copy command
		VkBufferAllocated instanceBuffer = CreateBufferWithInstanceData(
			commandBuffer,
			_mkDeviceRef.GetVmaAllocator(),
			tlases,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VMA_MEMORY_USAGE_GPU_ONLY,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
		);
		VkDeviceAddress instanceBufferAddr = _mkDeviceRef.GetBufferDeviceAddress(instanceBuffer.buffer);

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

		vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), scratchBuffer.buffer, scratchBuffer.allocation);
		vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), instanceBuffer.buffer, instanceBuffer.allocation);
	});
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
		_mkDeviceRef.GetDevice(),
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
		_mkDeviceRef.GetVmaAllocator(),
		buildSizeInfo.buildScratchSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	VkDeviceAddress scratchAddress = _mkDeviceRef.GetBufferDeviceAddress(scratchBuffer.buffer);

	// update build info 
	geoBuildInfo.srcAccelerationStructure = update ? _tlas.handle : VK_NULL_HANDLE;
	geoBuildInfo.dstAccelerationStructure = _tlas.handle;
	geoBuildInfo.scratchData.deviceAddress = scratchAddress;

	VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{ instanceCount, 0, 0, 0 };
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

	// Build the TLAS
	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &geoBuildInfo, &pBuildOffsetInfo);
}

/**
* Helpers
*/

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

void MKRaytracer::DestroyAccelerationStructureKHR(VkAccelKHR& accelStruct)
{
	vkDestroyAccelerationStructureKHR(_mkDeviceRef.GetDevice(), accelStruct.handle, nullptr);
	vmaDestroyBuffer(_mkDeviceRef.GetVmaAllocator(), accelStruct.buffer.buffer, accelStruct.buffer.allocation);
}