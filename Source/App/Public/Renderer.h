#pragma once

#include <assert.h>

// internal
#include "Utilities.h"
#include "Window.h"
#include "Instance.h"
#include "ValidationLayer.h"
#include "Device.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"
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
	void CreateRenderPass();
	void CreateImageSampler();
	void CreateDescriptorSet();
	void AppendDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32 count);

	/* update */
	void UpdateUniformBuffer();
	void UpdateModelDescriptor();
	void Update();

	/* draw */
	void RecordFrameBufferCommands(uint32 swapchainImageIndex);
	void Rasterize();

	/* cleanup */
	void Cleanup();

private:
	/* RHI Instance */
	MKWindow			_mkWindow;
	MKInstance			_mkInstance;
	MKDevice			_mkDevice;
	MKSwapchain	        _mkSwapchain;
	MKGraphicsPipeline	_mkGraphicsPipeline;

	/* source primitives */
	OBJModel _vikingModel;

	/* render pass */
	VkRenderPass _vkRenderPass;

	/* buffers */
	VkBufferAllocated _vkVertexBuffer;
	VkBufferAllocated _vkIndexBuffer;

	/* uniform buffer objects */
	std::vector<VkBufferAllocated>  _vkUniformBuffers;

	/* descriptor */
	VkDescriptorSetLayout         _vkDescriptorSetLayout;
	std::vector<VkDescriptorSet>  _vkDescriptorSets;

	/* camera */
	FreeCamera _camera;

	/* input controller */
	InputController _inputController;

	/* timer */
	Timer _timer{};

private:
	/* per frame member */
	uint32 _currentFrameIndex = 0;
	
	/* descriptor binding offset */
	uint32 _bindingOffset = 0;
	std::unordered_map<VkDescriptorType, uint32> _descriptorTypeMap;
};

