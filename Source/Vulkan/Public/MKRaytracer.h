#pragma once

#include "Utilities.h"
#include "MKCommandService.h"
#include "MKDescriptorManager.h"
#include "MKOffscreenRenderPass.h"
#include "OBJModel.h"

class MKGraphicsPipeline;

struct ScractchBuffer
{
	uint64         deviceAddress;
	VkBuffer       handle;
	VkDeviceMemory memory;
};

struct AcceleraationStructure
{
	uint64                             deviceAddress;
	VkAccelerationStructureKHR         handle;
	std::unique_ptr<VkBufferAllocated> buffer; // alllocated by VMA
};

struct StorageImage
{	
	VkDeviceMemory memory;
	VkImage        image;
	VkImageView    imageView;
	VkFormat       format;
	uint32         width;
	uint32         height;
};

struct UniformBufferObject
{
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
};

class MKRaytracer
{
public:
	MKRaytracer(MKRaytracer const&) = delete;            // remove copy constructor
	MKRaytracer& operator=(MKRaytracer const&) = delete; // remove copy assignment operator
	MKRaytracer() = default;
	~MKRaytracer();
	
	/* instance build */
	void   BuildRayTracer(MKDevice* devicePtr, const OBJModel& model, const std::vector<OBJInstance>& instances, VkDeviceAddress vertexAddr, VkDeviceAddress indexAddr, VkExtent2D extent);
	/* loader for getting proxy address of extended functions */
	void   LoadVkRaytracingExtension();
	
	/* BLAS */
	

	/* TLAS */

	/* storage image */
	void CreateStorageImage(VkExtent2D extent);
	
	/* shader binding table */
	void CreateShaderBindingTable();

	/* ray tracing descriptor set */
	void InitializeRayTracingDescriptorSet(VkStorageImage& storageImage);
	void UpdateDescriptorImageWrite(VkStorageImage& storageImage);

	/* ray tracing pipeline & shaders */
	void CreateRayTracingPipeline(VkDescriptorSetLayout externalDescriptorSetLayout);

	/* ray tracing */
	void TraceRay(VkCommandBuffer commandBuffer, const glm::vec4 clearColor, VkDescriptorSet externDescSet, VkPushConstantRaster rasterConstant, VkExtent2D extent);
	
	/* helpers */
	bool                                             HasFlag(VkFlags item, VkFlags flag);
	void						                     DestroyAccelerationStructureKHR(VkAccelKHR& accelStruct);
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  GetRayTracingPipelineProperties();
	inline VkTransformMatrixKHR                      ConvertGLMToVkMat4(const glm::mat4& glmMat4);
	inline uint32                                    GetAlignedSize(uint32 value, uint32 alignment) { return (value + alignment - 1) & ~(alignment - 1); }
	uint32                                           FindMemoryType(uint32 bits, VkMemoryPropertyFlags properties, VkBool32* memoryTypeFound=(VkBool32*)nullptr);

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
	PFN_vkCreateRayTracingPipelinesKHR                 vkCreateRayTracingPipelinesKHR = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesKHR           vkGetRayTracingShaderGroupHandlesKHR = nullptr;
	PFN_vkCmdTraceRaysKHR                              vkCmdTraceRaysKHR = nullptr;

private:
	/* device reference */
	MKDevice*            _mkDevicePtr;
	uint32               _graphicsQueueIndex;

	// storage image
	StorageImage		  _storageImage;
	VkBufferAllocated 	  _ubo;

	// vertex and index buffer
	VkBufferAllocated     _vertexBuffer;
	VkBufferAllocated     _indexBuffer;
	

	/* acceleration structures */
	AcceleraationStructure _blas;
	AcceleraationStructure _tlas;

	/* descriptor resources */
	VkDescriptorSetLayout         _vkRayTracingDescriptorSetLayout;
	std::vector<VkDescriptorSet>  _vkRayTracingDescriptorSet;

	/* ray tracing pipeline related */
	VkPipeline                                         _vkRayTracingPipeline;
	VkPipelineLayout                                   _vkRayTracingPipelineLayout;

	/* shader binding table */
	std::vector<VkRayTracingShaderGroupCreateInfoKHR>  _vkShaderGroupsInfo;
	VkBufferAllocated _sbtBuffer;
	VkStridedDeviceAddressRegionKHR _raygenRegion;
	VkStridedDeviceAddressRegionKHR _raymissRegion;
	VkStridedDeviceAddressRegionKHR _rayhitRegion;
	VkStridedDeviceAddressRegionKHR _callableRegion;
};