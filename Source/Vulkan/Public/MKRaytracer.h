#pragma once

#include "Utilities.h"
#include "MKCommandService.h"
#include "MKGraphicsPipeline.h"

class MKGraphicsPipeline;

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

	void   BuildRayTracer(const OBJModel& model, const std::vector<OBJInstance>& instances);
	/* loader for getting proxy address of extended functions */
	void   LoadVkRaytracingExtension();
	/* converter from obj model to geometry */
	VkBLAS ObjectToVkGeometryKHR(const OBJModel& model);
	/* getters*/
	VkDeviceAddress GetBLASDeviceAddress(uint32 index);
	
	/**
	* acceleration structure building
	*/
	VkAccelKHR CreateAccelerationStructureKHR(VkAccelerationStructureCreateInfoKHR& accelCreateInfo);
	
	/* BLAS */
	void   CreateBLAS(const OBJModel& model);
	void   BuildBLAS(const std::vector<VkBLAS>& blases, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	void   CmdCreateBLAS(
		VkCommandBuffer commandBuffer, 
		std::vector<uint32>& indices, 
		std::vector<VkAccelerationStructureKHRInfo>& buildAsInfo, 
		VkDeviceAddress scratchAddress, 
		VkQueryPool queryPool
	);
	void   CmdCreateCompactBLAS(
		VkCommandBuffer commandBuffer, 
		std::vector<uint32> indices, 
		std::vector<VkAccelerationStructureKHRInfo>& buildAsInfo, 
		VkQueryPool queryPool
	);
	void   DestroyNonCompactedBLAS(std::vector<uint32> indices, std::vector<VkAccelerationStructureKHRInfo>& buildAsInfo);

	/* TLAS */
	void   CreateTLAS(const std::vector<OBJInstance> instances);
	void   BuildTLAS(
		const std::vector<VkAccelerationStructureInstanceKHR>& tlases, 
		VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, 
		bool isUpdated = false
	);
	void   CmdCreateTLAS(
		VkCommandBuffer commandBuffer,
		uint32 instanceCount,
		VkDeviceAddress instanceBufferDeviceAddr,
		VkBufferAllocated& scratchBuffer,
		VkBuildAccelerationStructureFlagsKHR flags,
		bool update
	);
	
	/* helpers */
	bool                         HasFlag(VkFlags item, VkFlags flag);
	inline VkTransformMatrixKHR  ConvertGLMToVkMat4(const glm::mat4& glmMat4);
	void                         DestroyAccelerationStructureKHR(VkAccelKHR& accelStruct);
	/* abstraction for buffer creation logic with staging buffer */
	template<typename T>
	VkBufferAllocated            CreateBufferWithInstanceData(
		VkCommandBuffer commandBuffer,
		const VmaAllocator& allocator,
		const std::vector<T>& wantToCopy,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags memoryAllocationFlags
	)
	{
		assert(!wantToCopy.empty());
		VkDeviceSize bufferSize = sizeof(wantToCopy[0]) * wantToCopy.size();
		VkBufferAllocated stagingBuffer = util::CreateBuffer(
			allocator,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_AUTO,
			VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT // following convention for staging buffer based on VMA valid usage
		);
		// copy data to staging buffer
		memcpy(stagingBuffer.allocationInfo.pMappedData, wantToCopy.data(), (size_t)bufferSize);
		VkBufferAllocated resultBuffer = util::CreateBuffer(
			allocator,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage,
			memoryUsage,
			memoryAllocationFlags
		);

		VkBufferCopy copyRegion{};
		copyRegion.size = bufferSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, resultBuffer.buffer, 1, &copyRegion);
		
		vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		return resultBuffer;
	}
	
	/* vulkan extension proxy function */
	PFN_vkGetAccelerationStructureBuildSizesKHR        vkGetAccelerationStructureBuildSizesKHR = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR     vkGetAccelerationStructureDeviceAddressKHR = nullptr;
	PFN_vkCreateAccelerationStructureKHR               vkCreateAccelerationStructureKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR            vkCmdBuildAccelerationStructuresKHR = nullptr;
	PFN_vkCmdWriteAccelerationStructuresPropertiesKHR  vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;
	PFN_vkDestroyAccelerationStructureKHR              vkDestroyAccelerationStructureKHR = nullptr;

private:
	/* device reference */
	MKDevice&            _mkDeviceRef;
	MKGraphicsPipeline&  _mkPipelineRef;
	uint32               _graphicsQueueIndex;

	/* BLAS */
	std::vector<VkAccelKHR> _blases;
	/* TLAS */
	VkAccelKHR              _tlas;
};