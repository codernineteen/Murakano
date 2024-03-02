#pragma once

#define USE_HLSL

// internal
#include "Utilities.h"
#include "Global.h"
#include "MKDevice.h"
#include "MKSwapchain.h"
#include "MKDescriptorManager.h"
#include "OBJModel.h"

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
    void            CreateSyncObjects();
    /* actual buffer creation logic */
    void            CreateVertexBuffer();
    /* index buffer creation*/
    void            CreateIndexBuffer();
    /* Frame buffer commands recording and calling command service interfaces */
    void            RecordFrameBuffferCommand(uint32 swapchainImageIndex);
    /* copy source buffer to destination buffer */
    void            CopyBufferToBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize);

private:
    /* pipeline instance */
	VkPipeline	              _vkGraphicsPipeline;
	VkPipelineLayout          _vkPipelineLayout;
    /* sync objects */
    std::vector<VkSemaphore>  _vkImageAvailableSemaphores;
    std::vector<VkSemaphore>  _vkRenderFinishedSemaphores;
    std::vector<VkFence>      _vkInFlightFences;
    /* vertex buffer */
    VkBuffer                  _vkVertexBuffer;
    VkDeviceMemory            _vkVertexBufferMemory;
    /* index buffer */
    VkBuffer                  _vkIndexBuffer;
    VkDeviceMemory            _vkIndexBufferMemory;

    OBJModel                  _vikingRoom;

private:
	MKDevice&            _mkDeviceRef;
    MKSwapchain&         _mkSwapchainRef;
    uint32               _currentFrame = 0;
};