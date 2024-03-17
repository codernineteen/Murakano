#pragma once

#include "Utilities.h"
#include "MKCommandService.h"

class MKRaytracer
{
	// only used as reference from the TLAS
	struct BLAS
	{
		/* The shape and type of the acceleration structure */
		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
	};

public:
	MKRaytracer(MKRaytracer const&) = delete;            // remove copy constructor
	MKRaytracer& operator=(MKRaytracer const&) = delete; // remove copy assignment operator
	MKRaytracer() = default;
	~MKRaytracer();

	void BuildRayTracer();
	void LoadVkRaytracingExtension(VkDevice device);

private:
	/* BLAS */
	std::vector<BLAS> _blases;

#ifdef VK_KHR_acceleration_structure
	//static PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
#endif
};