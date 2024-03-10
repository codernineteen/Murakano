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
    void            UpdateUniformBuffer();

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

private:
	MKDevice&     _mkDeviceRef;
    MKSwapchain&  _mkSwapchainRef;
    uint32        _currentFrame = 0;
};