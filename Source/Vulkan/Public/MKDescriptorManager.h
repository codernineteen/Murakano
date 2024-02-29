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
	MKDescriptorManager();
	~MKDescriptorManager();
	void InitDescriptorManager(MKDevice* mkDevicePtr, VkExtent2D swapchainExtent);

	/* getters */
	VkDescriptorSetLayout*  GetDescriptorSetLayoutPtr() { return &_vkDescriptorSetLayout; }
	VkDescriptorSet*        GetDescriptorSetPtr(uint32 currentFrame) { return &_vkDescriptorSets[currentFrame]; }
	VkImageView             GetDepthImageView() const { return _vkDepthImageView; }

	/* undate uniform buffer objects state*/
	void                   UpdateUniformBuffer(uint32 currentFrame);
	/* update swapchain extent whenever there is recreation of it. */
	void                   UpdateSwapchainExtent(VkExtent2D swapchainExtent) { _vkSwapchainExtent = swapchainExtent; }

	/* transition image layout with pipeline barrier (when VK_SHARING_MODE_EXCLUSIVE) */
	void                   TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void                   CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	/* used in recreating swapchain */
	void                   CreateDepthResources();
	void                   DestroyDepthResources();

private:
	/* create uniform buffer object*/
	void CreateUniformBuffer();

	/* create texture image and view */
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();

	/* create depth buffer resources */
	bool HasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }


private:
	/* descriptor pool */
	VkDescriptorPool              _vkDescriptorPool = VK_NULL_HANDLE;

	/* uniform buffer descriptor*/
	VkDescriptorSetLayout         _vkDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkBuffer>         _vkUniformBuffers = std::vector<VkBuffer>();
	std::vector<VkDeviceMemory>   _vkUniformBuffersMemory = std::vector<VkDeviceMemory>();
	std::vector<void*>            _vkUniformBuffersMapped = std::vector<void*>();
	std::vector<VkDescriptorSet>  _vkDescriptorSets = std::vector<VkDescriptorSet>();

	/* extent reference */
	VkExtent2D                    _vkSwapchainExtent = VkExtent2D{ 0, 0 };

	/* texture resources */
	VkImage                       _vkTextureImage = VK_NULL_HANDLE;
	VkImageView                   _vkTextureImageView = VK_NULL_HANDLE;
	VkDeviceMemory                _vkTextureImageMemory = VK_NULL_HANDLE;
	VkSampler                     _vkTextureSampler = VK_NULL_HANDLE;

	/* depth buffer resources */
	VkImage                       _vkDepthImage = VK_NULL_HANDLE;
	VkImageView                   _vkDepthImageView = VK_NULL_HANDLE;
	VkDeviceMemory                _vkDepthImageMemory = VK_NULL_HANDLE;

private:
	MKDevice* _mkDevicePtr = nullptr;
};