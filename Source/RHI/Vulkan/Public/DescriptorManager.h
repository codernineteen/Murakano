#pragma once

// external
#include <chrono> // for updating uniform buffer object state

// internal
#include "Utilities.h"
#include "UniformBuffer.h"
#include "CommandService.h"

class MKDescriptorManager
{
public:
	MKDescriptorManager();
	~MKDescriptorManager();
	void InitDescriptorManager(MKDevice* mkDevicePtr);

	/* commands - transition image layout with pipeline barrier (when VK_SHARING_MODE_EXCLUSIVE) */
	void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height); 

	/* descriptor manager api */
	VkDescriptorPool       GetDescriptorPool();
	VkDescriptorPool       CreateDescriptorPool(uint32 setCount);
	void                   AddDescriptorSetLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags, uint32_t binding, uint32_t descriptorCount);
	void                   CreateDescriptorSetLayout(VkDescriptorSetLayout& layout);
	void                   AllocateDescriptorSet(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorSetLayout layout);
	void                   ResetDescriptorPool();
	
	/* write api */
	void                   WriteBufferToDescriptorSet(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32 dstBinding, VkDescriptorType descriptorType);
	void                   WriteImageToDescriptorSet(VkImageView imageView, VkImageLayout imageLayout, uint32 dstBinding, VkDescriptorType descriptorType);
	void                   WriteImageArrayToDescriptorSet(VkDescriptorImageInfo* imageInfosPtr, uint32 sizeOfArray, uint32 dstBinding, VkDescriptorType descriptorType);
	void                   WriteSamplerToDescriptorSet(VkSampler sampler, uint32 dstBinding, VkDescriptorType descriptorType);
	void                   WriteAccelerationStructureToDescriptorSet(const VkAccelerationStructureKHR* as, uint32 dstBinding);
	void                   WriteCombinedImageSamplerToDescriptorSet(VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, uint32 dstBinding);
	void                   UpdateDescriptorSet(VkDescriptorSet descriptorSet);

private:
	/* create depth buffer resources */
	bool HasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }


private:
	/* descriptor pool sizes */
	std::vector<VkDescriptorPoolSize> _vkDescriptorPoolSizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,MAX_FRAMES_IN_FLIGHT},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_FRAMES_IN_FLIGHT * 10},
		{VK_DESCRIPTOR_TYPE_SAMPLER, MAX_FRAMES_IN_FLIGHT * 2}
	};

	/* waiting layout bindings */
	std::vector<VkDescriptorSetLayoutBinding>                   _vkWaitingBindings    = std::vector<VkDescriptorSetLayoutBinding>();
	
	/* descriptor info shared pointers */
	std::unordered_set<std::shared_ptr<VkDescriptorBufferInfo>> _vkWaitingBufferInfos = std::unordered_set<std::shared_ptr<VkDescriptorBufferInfo>>();
	std::unordered_set<std::shared_ptr<VkDescriptorImageInfo>>  _vkWaitingImageInfos  = std::unordered_set<std::shared_ptr<VkDescriptorImageInfo>>();
	
	std::unordered_set<std::shared_ptr<VkWriteDescriptorSetAccelerationStructureKHR>> _vkWaitingAsWrites = std::unordered_set<std::shared_ptr<VkWriteDescriptorSetAccelerationStructureKHR>>();
	std::vector<VkWriteDescriptorSet>                           _vkWaitingWrites = std::vector<VkWriteDescriptorSet>();

	std::vector<VkDescriptorPool> _vkDescriptorPoolReady; // available descriptor pool
	std::vector<VkDescriptorPool> _vkDescriptorPoolFull;  // full descriptor pool 

	/* max set count per pool */
	uint32 _setsPerPool = 100; // default is 100, max is 4092

private:
	MKDevice* _mkDevicePtr = nullptr;
};