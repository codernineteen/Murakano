#pragma once

// internal
#include "Utilities.h"
#include "Global.h"
#include "Vertex.h"
#include "MKDevice.h"
#include "MKSwapchain.h"

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
    /* a helper function to create a Vkbuffer,allocate its memory and binding them together */
    void            CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    /* actual buffer creation logic */
    void            CreateVertexBuffer();
    /* index buffer creation*/
    void            CreateIndexBuffer();
    /* Frame buffer commands recording and calling command service interfaces */
    void            RecordFrameBuffferCommand(uint32 swapchainImageIndex);
    /* type filtering for memoryTypebits and find appropriate target depending on memory property flags */
    uint32          FindMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties);
    /* copy buffer to another buffer */
    void            CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

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

private:
	MKDevice&     _mkDeviceRef;
    MKSwapchain&  _mkSwapchainRef;
    uint32        _currentFrame = 0;
};