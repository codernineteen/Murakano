#pragma once

#include "Utilities.h"
#include "MKCommandService.h"
#include "MkGraphicsPipeline.h"

class MKRaytracer
{
	// only used as reference from the TLAS
	struct BLAS
	{
		/* The shape and type of the acceleration structure */
		VkAccelerationStructureGeometryKHR        geometry;
		VkAccelerationStructureBuildRangeInfoKHR  buildRangeInfo;
	};

public:
	MKRaytracer(MKRaytracer const&) = delete;            // remove copy constructor
	MKRaytracer& operator=(MKRaytracer const&) = delete; // remove copy assignment operator
	MKRaytracer(MKDevice& mkDeviceRef, MKGraphicsPipeline& mkPipelineRef);
	~MKRaytracer();

	void BuildRayTracer();
	void LoadVkRaytracingExtension();
	BLAS ObjectToVkGeometryKHR(const OBJModel& model);

private:
	/* device reference */
	MKDevice&            _mkDeviceRef;
	MKGraphicsPipeline&  _mkPipelineRef;
	uint32               _graphicsQueueIndex;

	/* BLAS */
	std::vector<BLAS> _blases;

#ifdef VK_KHR_acceleration_structure
	//static PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
#endif
};