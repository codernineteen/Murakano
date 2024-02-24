#pragma once

#define GLM_FORCE_RADIANS

// external
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono> // for updating uniform buffer object state

// internal
#include "Conversion.h"
#include "Utilities.h"
#include "UniformBuffer.h"
#include "MKCommandService.h"

class MKDescriptor
{
public:
	MKDescriptor(MKDevice& mkDeviceRef, const VkExtent2D& swapchainExtent);
	~MKDescriptor();

	void                   UpdateUniformBuffer(uint32 currentFrame);
	VkDescriptorSetLayout* GetDescriptorSetLayoutPtr() { return &_vkDescriptorSetLayout; }
	VkDescriptorSet*       GetDescriptorSetPtr(uint32 currentFrame) { return &_vkDescriptorSets[currentFrame]; }

private:
	/* descriptor pool */
	VkDescriptorPool                    _vkDescriptorPool;

	/* uniform buffer descriptor*/
	VkDescriptorSetLayout         _vkDescriptorSetLayout;
	std::vector<VkBuffer>         _vkUniformBuffers;
	std::vector<VkDeviceMemory>   _vkUniformBuffersMemory;
	std::vector<void*>            _vkUniformBuffersMapped;
	std::vector<VkDescriptorSet>  _vkDescriptorSets;
	
	const VkExtent2D&             _vkSwapchainExtent;

private:
	MKDevice& _mkDeviceRef;
};