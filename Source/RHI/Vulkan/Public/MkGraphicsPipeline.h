#pragma once

#define USE_HLSL

// internal
#include "Utilities.h"
#include "Global.h"
#include "MKDevice.h"
#include "MKSwapchain.h"
#include "MKDescriptorManager.h"
#include "OBJModel.h"
#include "Texture.h"
#include "FreeCamera.h"
#include "InputController.h"

// [MKGraphicsPipeline class]
// - Responsibility :
//    - specify graphics pipeline.
// - Dependency :
//    - MKDevice as reference
//    - MKRenderPass as member
class MKGraphicsPipeline 
{
public:
	MKGraphicsPipeline(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef);
	~MKGraphicsPipeline();

    /* draw frames */
    void DrawFrame();

private:
	VkShaderModule  CreateShaderModule(const std::vector<char>& code);
    /* create synchronization objects */
    void            CreateSyncObjects();
    /* actual buffer creation logic */
    void            CreateVertexBuffer();
    /* index buffer creation*/
    void            CreateIndexBuffer();
    /* uniform buffer creation */
    void            CreateUniformBuffers();
    /* Frame buffer commands recording and calling command service interfaces */
    void            RecordFrameBuffferCommand(uint32 swapchainImageIndex);
    /* copy source buffer to destination buffer */
    void            CopyBufferToBuffer(VkBufferAllocated src, VkBufferAllocated dest, VkDeviceSize size);
    /* update uniform buffer */
    void            UpdateUniformBuffer(float elapsedTime);

private:
    /* pipeline instance */
	VkPipeline	      _vkGraphicsPipeline;
	VkPipelineLayout  _vkPipelineLayout;
    
    /* sync objects */
    std::vector<VkSemaphore>  _vkImageAvailableSemaphores;
    std::vector<VkSemaphore>  _vkRenderFinishedSemaphores;
    std::vector<VkFence>      _vkInFlightFences;
    
    /* vertex buffer */
    VkBufferAllocated _vkVertexBuffer;
    
    /* index buffer */
    VkBufferAllocated _vkIndexBuffer;

    /* descriptor set */
    VkDescriptorSetLayout         _vkDescriptorSetLayout;
    std::vector<VkDescriptorSet>  _vkDescriptorSets;

    /* uniform buffer objects */
    std::vector<VkBufferAllocated>  _vkUniformBuffers;
    std::vector<void*>              _vkUniformBuffersMappedData;

    /* 3d model */
    OBJModel _vikingRoom;

    /* camera */
    FreeCamera _camera;

    /* input controller */
    InputController _inputController;

private:
	MKDevice&     _mkDeviceRef;
    MKSwapchain&  _mkSwapchainRef;
    uint32        _currentFrame = 0;
    float         _deltaTime = 0.0f;
    std::chrono::steady_clock::time_point  _lastTime = std::chrono::high_resolution_clock::now();
};