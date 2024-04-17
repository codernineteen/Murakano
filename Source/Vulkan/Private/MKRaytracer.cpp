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
	vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(_mkDevicePtr->GetDevice(), "vkCmdTraceRaysKHR");
#else
	MK_THROW("VK_KHR_accleration_structure is not enabled.");
#endif
}

void MKRaytracer::CreateStorageImage(VkExtent2D extent) 
{
	_storageImage.width = extent.width;
	_storageImage.height = extent.height;

	VkImageCreateInfo imageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent = { extent.width , extent.height, 1 }; // width, height, depth
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateImage(_mkDevicePtr->GetDevice(), &imageCreateInfo, nullptr, &_storageImage.image);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_mkDevicePtr->GetDevice(), _storageImage.image, &memRequirements); // populate memory requirements
	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	MK_CHECK(vkAllocateMemory(_mkDevicePtr->GetDevice(), &allocInfo, nullptr, &_storageImage.memory));
	MK_CHECK(vkBindImageMemory(_mkDevicePtr->GetDevice(), _storageImage.image, _storageImage.memory, 0));

	VkImageViewCreateInfo colorImageView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;
	colorImageView.image = _storageImage.image;
	MK_CHECK(vkCreateImageView(_mkDevicePtr->GetDevice(), &colorImageView, nullptr, &_storageImage.imageView));

	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); // transient command pool for one-time command buffer

	VkCommandBuffer commandBuffer;
	GCommandService->BeginSingleTimeCommands(commandBuffer, cmdPool);

	util::TransitionImageLayoutVerbose(
		commandBuffer,
		_storageImage.image,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkAccessFlags(), VkAccessFlags(),
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	);

	GCommandService->EndSingleTimeCommands(commandBuffer, cmdPool);	
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

	// handle size and count
	const uint32 handleSize = rayTrackingPipelineProperties.shaderGroupHandleSize;
	const uint32 handleCount = 1 + missCount + hitCount;

	const uint32 handleSizeAligned = GetAlignedSize(handleSize, rayTrackingPipelineProperties.shaderGroupHandleAlignment);

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
	uint32 dataSize = handleCount * handleSize;
	std::vector<uint8> shaderHandles(dataSize);
	MK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(_mkDevicePtr->GetDevice(), _vkRayTracingPipeline, 0, handleCount, dataSize, shaderHandles.data()));

	// create shader binding table buffer that hold 'handle data'
	VkDeviceSize sbtSize = _raygenRegion.size + _raymissRegion.size + _rayhitRegion.size + _callableRegion.size;
	const VkBufferUsageFlags sbtUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
	_sbtBuffer = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
		sbtSize,
		sbtUsageFlags,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // host visible, host coherent
		0 // no memory allocation flags
	);

	// store device address offset for each shader region.
	auto sbtBufferDeviceAddress = _mkDevicePtr->GetBufferDeviceAddress(_sbtBuffer.buffer);
	_raygenRegion.deviceAddress = sbtBufferDeviceAddress;
	_raymissRegion.deviceAddress = sbtBufferDeviceAddress + _raygenRegion.size;
	_rayhitRegion.deviceAddress = sbtBufferDeviceAddress + _raygenRegion.size + _raymissRegion.size;

	void* voidData;
	vmaMapMemory(_mkDevicePtr->GetVmaAllocator(), _sbtBuffer.allocation, &voidData);
	uint8* pSbtBuffer = reinterpret_cast<uint8*>(voidData);
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

	// unmap memory
	vmaUnmapMemory(_mkDevicePtr->GetVmaAllocator(), _sbtBuffer.allocation);
	pSbtBuffer = nullptr;
}

void MKRaytracer::TraceRay(VkCommandBuffer commandBuffer, const glm::vec4 clearColor, VkDescriptorSet externDescSet, VkPushConstantRaster rasterConstant, VkExtent2D extent)
{
	// initialize push constant for ray tracing
	_vkRayTracingPushConstant.clearColor = clearColor;
	_vkRayTracingPushConstant.lightIntensity = rasterConstant.lightIntensity;
	_vkRayTracingPushConstant.lightPos = rasterConstant.lightPosition;
	_vkRayTracingPushConstant.lightType = rasterConstant.lightType;

	/**
	* populate descriptor sets
	* - set 0 : ray tracing descriptor set
	* - set 1 : external descriptor set
	*/
	std::vector<VkDescriptorSet> descriptorSets = { _vkRayTracingDescriptorSet.front(), externDescSet};
	// bind ray tracing pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _vkRayTracingPipeline);
	// bind descriptor sets
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
		_vkRayTracingPipelineLayout, 
		0, 
		static_cast<uint32>(descriptorSets.size()), 
		descriptorSets.data(), 
		0, 
		nullptr
	);
	// bind push constants
	vkCmdPushConstants(
		commandBuffer,
		_vkRayTracingPipelineLayout,
		VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
		0,
		sizeof(VkPushConstantRay),
		&_vkRayTracingPushConstant
	);

	// record ray trace command
	vkCmdTraceRaysKHR(
		commandBuffer,
		&_raygenRegion,
		&_raymissRegion,
		&_rayhitRegion,
		&_callableRegion,
		extent.width,
		extent.height,
		1
	);
}


/**
* --------------------- Helpers ---------------------------
*/

bool MKRaytracer::HasFlag(VkFlags item, VkFlags flag)
{
	return (item & flag) == flag;
}


// find memory type
uint32 MKRaytracer::FindMemoryType(uint32 bits, VkMemoryPropertyFlags properties, VkBool32* memoryTypeFound) {
	VkPhysicalDeviceMemoryProperties deviceMemProperties;
	vkGetPhysicalDeviceMemoryProperties(_mkDevicePtr->GetPhysicalDevice(), &deviceMemProperties);

	for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((deviceMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memoryTypeFound)
				{
					*memoryTypeFound = true;
				}
				return i;
			}
		}
		bits >>= 1;
	}

	if (memoryTypeFound)
	{
		*memoryTypeFound = false;
		return 0;
	}
	else
	{
		MK_THROW("Could not find a matching memory type");
	}
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