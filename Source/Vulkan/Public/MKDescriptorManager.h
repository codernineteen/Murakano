#pragma once

// external
#include <chrono> // for updating uniform buffer object state
#include <stb_image.h>

// internal
#include "Conversion.h"
#include "Utilities.h"
#include "UniformBuffer.h"
#include "MKCommandService.h"

class MKDescriptorManager
{
public:
	MKDescriptorManager(MKDevice& mkDeviceRef, const VkExtent2D& swapchainExtent);
	~MKDescriptorManager();

	/* getters */
	VkDescriptorSetLayout* GetDescriptorSetLayoutPtr() { return &_vkDescriptorSetLayout; }
	VkDescriptorSet*       GetDescriptorSetPtr(uint32 currentFrame) { return &_vkDescriptorSets[currentFrame]; }

	/* undate uniform buffer objects state*/
	void                   UpdateUniformBuffer(uint32 currentFrame);

	/* transition image layout with pipeline barrier (when VK_SHARING_MODE_EXCLUSIVE) */
	void                   TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void                   CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

private:
	/* create texture image and view */
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();


private:
	/* descriptor pool */
	VkDescriptorPool              _vkDescriptorPool;

	/* uniform buffer descriptor*/
	VkDescriptorSetLayout         _vkDescriptorSetLayout;
	std::vector<VkBuffer>         _vkUniformBuffers;
	std::vector<VkDeviceMemory>   _vkUniformBuffersMemory;
	std::vector<void*>            _vkUniformBuffersMapped;
	std::vector<VkDescriptorSet>  _vkDescriptorSets;
	
	/* Extent reference */
	const VkExtent2D&             _vkSwapchainExtent;

	/* Texture image and view */
	VkImage                       _vkTextureImage;
	VkImageView                   _vkTextureImageView;
	VkDeviceMemory                _vkTextureImageMemory;
	VkSampler                     _vkTextureSampler;

private:
	MKDevice& _mkDeviceRef;
};