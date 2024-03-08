#pragma once

// external
#include <chrono> // for updating uniform buffer object state
#include <stb_image.h>

// internal
#include "Utilities.h"
#include "UniformBuffer.h"
#include "MKCommandService.h"

class MKDescriptorManager
{
public:
	MKDescriptorManager();
	~MKDescriptorManager();
	void InitDescriptorManager(MKDevice* mkDevicePtr, VkExtent2D swapchainExtent);

	/* getters */
	VkDescriptorSetLayout*  GetDescriptorSetLayoutPtr() { return &_vkDescriptorSetLayout; }
	VkDescriptorSet*        GetDescriptorSetPtr(uint32 currentFrame) { return &_vkDescriptorSets[currentFrame]; }
	VkImageView             GetDepthImageView() const { return _vkDepthImageView; }

	/* undate uniform buffer objects state*/
	void UpdateUniformBuffer(uint32 currentFrame);
	
	/* update swapchain extent whenever there is recreation of it. */
	void UpdateSwapchainExtent(VkExtent2D swapchainExtent) { _vkSwapchainExtent = swapchainExtent; }

	/* commands - transition image layout with pipeline barrier (when VK_SHARING_MODE_EXCLUSIVE) */
	void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height); 

	/* api */
	VkDescriptorPool GetDescriptorPool();
	VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);
	void AddDescriptorSetLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags, uint32_t binding, uint32_t descriptorCount);
	VkDescriptorSetLayout CreateDescriptorSetLayout();
	VkDescriptorPool CreateDescriptorPool(std::vector<VkDescriptorPoolSizeRatio> descriptorPoolSizeRatios, uint32 setCount);
	void ResetDescriptorPool();

	/* create resources */
	void CreateTextureImage(const std::string texturePath, VkImageAllocated& textureImage);
	void CreateTextureImageView(VkImage& textureImage, VkImageView& textureImageView);
	void CreateTextureSampler();
	void CreateDepthResources();

	/* destroy resources */
	void DestroyTextureImage(VkImageAllocated textureImage) { vmaDestroyImage(_mkDevicePtr->GetVmaAllocator(), textureImage.image, textureImage.allocation); }
	void DestroyTextureImageView(VkImageView textureImageView) { vkDestroyImageView(_mkDevicePtr->GetDevice(), textureImageView, nullptr); }
	void DestroyTextureSampler(VkSampler textureSampler) { vkDestroySampler(_mkDevicePtr->GetDevice(), textureSampler, nullptr); } 
	void DestroyDepthResources();

private:
	/* create uniform buffer object*/
	void CreateUniformBuffer();

	/* create depth buffer resources */
	bool HasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }


private:
	/* descriptor pool */
	VkDescriptorPool _vkDescriptorPool = VK_NULL_HANDLE;

	std::vector<VkDescriptorSetLayoutBinding> _vkWaitingBindings;

	std::vector<VkDescriptorPoolSizeRatio>  _vkDescriptorPoolSizeRatios;
	std::vector<VkDescriptorPool>           _vkDescriptorPoolReady;
	std::vector<VkDescriptorPool>           _vkDescriptorPoolFull;

	/* uniform buffer descriptor*/
	VkDescriptorSetLayout         _vkDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkBufferAllocated>_vkUniformBuffers = std::vector<VkBufferAllocated>();
	std::vector<void*>            _vkUniformBuffersMapped = std::vector<void*>();
	std::vector<VkDescriptorSet>  _vkDescriptorSets = std::vector<VkDescriptorSet>();

	/* extent reference */
	VkExtent2D                    _vkSwapchainExtent = VkExtent2D{ 0, 0 };

	/* texture resources */
	VkImageAllocated              _vkTextureImage;
	VkImageView                   _vkTextureImageView = VK_NULL_HANDLE;
	VkSampler                     _vkTextureSampler = VK_NULL_HANDLE;

	/* depth buffer resources */
	VkImageAllocated              _vkDepthImage;
	VkImageView                   _vkDepthImageView = VK_NULL_HANDLE;

	/* default set count per pool */
	uint32                        _setsPerPool = 100;

private:
	MKDevice* _mkDevicePtr = nullptr;
};