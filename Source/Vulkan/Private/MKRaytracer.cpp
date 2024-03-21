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

MKRaytracer::BLAS MKRaytracer::ObjectToVkGeometryKHR(const OBJModel& model)
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

	BLAS blas{ geometry, buildRangeInfo };
	return blas;
}

void MKRaytracer::LoadVkRaytracingExtension()
{
	//vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
}