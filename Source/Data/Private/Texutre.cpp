#include "Texture.h"

#include "VulkanType.h"

Texture::Texture()
{
}

Texture::~Texture()
{
}

void Texture::BuildTextureFromExternal(MKDevice& device, const std::string& name, const std::string& path)
{
    texturePath = path;
    CreateTextureImage(name);
    CreateTextureImageView(device);
}

void Texture::CreateTextureImage(const std::string& name)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels        = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth * texHeight * 4);

    if (!pixels)
        MK_THROW("failed to load texture image!");

    // same manner as vertex buffer creation
    VkBufferAllocated stagingBuffer;
    GAllocator->CreateBuffer(
        &stagingBuffer,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        "staging buffer(" + name + ")"
    );

    // map the buffer memory to copy the pixel data into it
    memcpy(stagingBuffer.allocationInfo.pMappedData, pixels, SafeStaticCast<size_t, uint32>(imageSize));

    // free after copying pixel data to staging buffer
    stbi_image_free(pixels);

    GAllocator->CreateImage(
        &image,
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,                                      // image usage -  renderer using staging buffer to copy pixel data
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // image properties - destination of buffer copy and sampled in the shader 
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        name
    );

    /**
    * layout transitions
    *  1. undefined -> transfer destination (initial layout transfer writes, no need to wait for anything)
    *  2. transfer destination -> shader reading (need to wait 'transfer writes')
    */
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    std::queue<VoidLambda> commandQueue;
    commandQueue.push([&](VkCommandBuffer commandBuffer) {
        mk::vk::TransitionImageLayout(
            commandBuffer,                            // command buffer
            image.image,                              // texture image
            VK_FORMAT_R8G8B8A8_SRGB,                  // format
            VK_IMAGE_LAYOUT_UNDEFINED,                // old layout
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // new layout
            subresourceRange
        );
    });
    commandQueue.push([&](VkCommandBuffer commandBuffer) {
        // copy image data from staging buffer to texture image after layout transition
        mk::vk::CopyBufferToImage(
            commandBuffer,
            stagingBuffer.buffer,
            image.image,
            static_cast<uint32>(texWidth),
            static_cast<int32>(texHeight)
        );
    });
    commandQueue.push([&](VkCommandBuffer commandBuffer) {
        mk::vk::TransitionImageLayout(
            commandBuffer,                            // command buffer
            image.image,                              // texture image
            VK_FORMAT_R8G8B8A8_SRGB,                  // format
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // old layout
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // new layout
            subresourceRange
        );
    });

    GCommandService->ExecuteCommands(commandQueue);


    // destroy staging buffer and free memory
    GAllocator->DestroyBuffer(stagingBuffer);
}

void Texture::CreateTextureImageView(MKDevice& device)
{
    mk::vk::CreateImageView(
        device.GetDevice(),
        image.image,
        imageView,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );
}

void Texture::DestroyTexture(MKDevice& device)
{
    // destroy image and its view
    GAllocator->DestroyImage(image);
    vkDestroyImageView(device.GetDevice(), imageView, nullptr);
}