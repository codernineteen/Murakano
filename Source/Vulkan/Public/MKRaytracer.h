#pragma once

#include "Utilities.h"
#include "MKCommandService.h"
#include "MkGraphicsPipeline.h"

class MKRaytracer
{
	struct VkAccelerationStructureKHRInfo 
	{
		VkAccelerationStructureBuildGeometryInfoKHR      buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		VkAccelerationStructureBuildSizesInfoKHR         sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		const VkAccelerationStructureBuildRangeInfoKHR*  rangeInfo;
		VkAccelKHR                                       accelStruct; // range
		VkAccelKHR                                       cleanupAS;
	};
public:
	MKRaytracer(MKRaytracer const&) = delete;            // remove copy constructor
	MKRaytracer& operator=(MKRaytracer const&) = delete; // remove copy assignment operator
	MKRaytracer(MKDevice& mkDeviceRef, MKGraphicsPipeline& mkPipelineRef);
	~MKRaytracer();

	void   BuildRayTracer();
	void   LoadVkRaytracingExtension();
	VkBLAS ObjectToVkGeometryKHR(const OBJModel& model);
	void   BuildBLAS(const std::vector<VkBLAS>& blases, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	void   CreateBLAS(const OBJModel& model);

private:
	/* device reference */
	MKDevice&            _mkDeviceRef;
	MKGraphicsPipeline&  _mkPipelineRef;
	uint32               _graphicsQueueIndex;

	/* BLAS */
	std::vector<VkBLAS> _blases;

#ifdef VK_KHR_acceleration_structure
	//static PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
#endif
};