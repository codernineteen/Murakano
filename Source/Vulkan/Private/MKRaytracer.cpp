#include "MKRaytracer.h"

MKRaytracer::MKRaytracer(MKDevice& mkDeviceRef, MKGraphicsPipeline& mkPipelineRef)
	:
	_mkDeviceRef(mkDeviceRef),
	_mkPipelineRef(mkPipelineRef)
{
	_graphicsQueueIndex = mkDeviceRef.FindQueueFamilies(mkDeviceRef.GetPhysicalDevice()).graphicsFamily.value();
}

void MKRaytracer::BuildRayTracer()
{
// BLAS
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
		asTotalSize += buildAs[idx].sizeInfo.accelerationStructureSize;
		maxScratchSize = std::max(maxScratchSize, buildAs[idx].sizeInfo.buildScratchSize);
		nbCompactions += hasFlag(buildAs[idx].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}
}

void MKRaytracer::LoadVkRaytracingExtension()
{
	//vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
}