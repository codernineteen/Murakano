#pragma once

#include <assert.h>

// internal
#include "Utilities.h"
#include "Window.h"
#include "Instance.h"
#include "ValidationLayer.h"
#include "Device.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "CommandService.h"
#include "Allocator.h"
#include "RenderPassUtil.h"

class Renderer
{
	struct Timer
	{
		std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();
		std::chrono::steady_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
		float elapsedTime = 0.0f;
		float deltaTime = 0.0f;

		void Start()
		{
			startTime = std::chrono::high_resolution_clock::now();
		}

		void Update()
		{
			startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			// update delta time
			deltaTime = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastTime).count();
			deltaTime /= 1000.0f;
			lastTime = currentTime;
		}
	};

public:
	Renderer();
	~Renderer();
	void Setup();
	void Render();

private: 
	/* initialization */
	void CreateVertexBuffer(std::vector<Vertex> vertices);
	void CreateIndexBuffer(std::vector<uint32> indices);
	void CreateUniformBuffers();
	void CreateOffscreenRenderResource(VkExtent2D extent);
	void CreateOffscreenRenderPass(VkExtent2D extent);
	void CreateBaseDescriptorSet();
	void CreateSamplerDescriptorSet();
	void CreatePostDescriptorSet();
	void CreatePushConstantRaster();
	void CreateFrameBuffers();

	/* destroyer */
	void DestroyOffscreenRenderingResources();
	void DestroyOffscreenRenderPassResources();
	void DestroyFrameBuffers();

	/* update */
	void UpdateUniformBuffer();
	void WriteBaseDescriptor();
	void WriteSamplerDescriptor();
	void WritePostDescriptor();
	void Update();
	void OnResizeWindow();

	/* draw */
	void RecordFrameBufferCommands(uint32 swapchainImageIndex);
	void Rasterize(const VkCommandBuffer& commandBuffer, VkExtent2D extent);
	void DrawPostProcess(const VkCommandBuffer& commandBuffer, VkExtent2D extent);
	void DrawFrame();

	/* cleanup */
	void Cleanup();

	/* helper */
	void CopyBufferToBuffer(VkBufferAllocated src, VkBufferAllocated dst, VkDeviceSize sz);
	bool IsDepthOnlyFormat(VkFormat format)
	{
		return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_X8_D24_UNORM_PACK32;
	}

private:
	/* RHI Instance */
	MKWindow	_mkWindow;
	MKInstance	_mkInstance;
	MKDevice	_mkDevice;
	MKSwapchain	_mkSwapchain;
	MKPipeline	_mkGraphicsPipeline;
	MKPipeline  _mkPostPipeline;

	/* device properties */
	VkPhysicalDeviceProperties _vkDeviceProperties;

	/* source primitives */
	OBJModel _objModel;

	/* offscreen render pass */
	VkFormat              _vkOffscreenColorFormat{ VK_FORMAT_R32G32B32A32_SFLOAT };
	VkFormat              _vkOffscreenDepthFormat{ VK_FORMAT_X8_D24_UNORM_PACK32 };
	VkImageAllocated      _vkOffscreenColorImage;
	VkImageView           _vkOffscreenColorImageView;
	VkSampler             _vkOffscreenColorSampler{ VK_NULL_HANDLE };
	VkImageAllocated      _vkOffscreenDepthImage;
	VkImageView           _vkOffscreenDepthImageView;
	VkDescriptorImageInfo _vkOffscreenColorDescriptorInfo;
	
	/* render pass resources */
	VkRenderPass _vkOffscreenRednerPass{ VK_NULL_HANDLE };
	VkRenderPass _vkRenderPass;
	
	VkFramebuffer              _vkOffscreenFramebuffer{ VK_NULL_HANDLE };
	std::vector<VkFramebuffer> _vkFramebuffers;

	/* buffers */
	VkBufferAllocated _vkVertexBuffer;
	VkBufferAllocated _vkIndexBuffer;

	/* uniform buffer objects */
	std::vector<VkBufferAllocated>  _vkUniformBuffers;

	/* descriptor */
	VkDescriptorSetLayout _vkBaseDescriptorSetLayout;
	VkDescriptorSetLayout _vkSamplerDescriptorSetLayout;
	VkDescriptorSetLayout _vkPostDescriptorSetLayout;
	std::vector<VkDescriptorSet>  _vkBaseDescriptorSets;
	std::vector<VkDescriptorSet>  _vkSamplerDescriptorSets;
	std::vector<VkDescriptorSet>  _vkPostDescriptorSets;

	/* image sampler */
	VkSampler _vkLinearSampler;

	/* push constants */
	VkPushConstantRaster             _vkPushConstantRaster;
	std::vector<VkPushConstantRange> _vkPushConstantRanges;

	/* camera */
	FreeCamera _camera;

	/* input controller */
	InputController _inputController;

	/* timer */
	Timer _timer{};

private:
	/* per frame member */
	uint32 _currentFrameIndex = 0;
};

