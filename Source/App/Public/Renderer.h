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
	void Rasterize();

private: 
	/* initialization */
	void CreateVertexBuffer(std::vector<Vertex> vertices);
	void CreateIndexBuffer(std::vector<uint32> indices);
	void CreateUniformBuffers();
	void CreateRenderPass();

	/* update */
	void UpdateUniformBuffer();
	void Update();

	/* draw */
	void RecordFrameBufferCommands(uint32 swapchainImageIndex);
	void Rasterize();
	void Render();

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
	//float  _deltaTime = 0.0f;

	///* system clock */
	//std::chrono::steady_clock::time_point  _lastTime = std::chrono::high_resolution_clock::now();
};

