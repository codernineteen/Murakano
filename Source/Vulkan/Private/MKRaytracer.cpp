#include "MKRaytracer.h"

MKRaytracer::MKRaytracer(MKDevice& mkDeviceRef)
	:
	_mkDeviceRef(mkDeviceRef)
{
	_graphicsQueueIndex = mkDeviceRef.FindQueueFamilies(mkDeviceRef.GetPhysicalDevice()).graphicsFamily.value();
}

void MKRaytracer::BuildRayTracer()
{
// BLAS

}

void MKRaytracer::LoadVkRaytracingExtension()
{
	//vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
}