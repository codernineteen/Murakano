#include "Texture.h"

#include "VulkanType.h"

Texture::Texture(MKDevice& mkDeviceRef, const std::string texturePath)
    : mkDeviceRef(mkDeviceRef), texturePath(texturePath)
{
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
}

Texture::~Texture()
{
    vmaDestroyImage(mkDeviceRef.GetVmaAllocator(), image.image, image.allocation);
    vkDestroyImageView(mkDeviceRef.GetDevice(), imageView, nullptr);
    vkDestroySampler(mkDeviceRef.GetDevice(), sampler, nullptr);
}

void Texture::CreateTextureImage()
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = SafeStaticCast<int, VkDeviceSize>(texWidth * texHeight * 4);

    if (!pixels)
        throw std::runtime_error("failed to load texture image!");

    // same manner as vertex buffer creation
    VkBufferAllocated stagingBuffer = util::CreateBuffer(
        mkDeviceRef.GetVmaAllocator(),
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
    );

    // map the buffer memory to copy the pixel data into it
    memcpy(stagingBuffer.allocationInfo.pMappedData, pixels, SafeStaticCast<size_t, uint32>(imageSize));

    // free after copying pixel data to staging buffer
    stbi_image_free(pixels);

    util::CreateImage(
        mkDeviceRef.GetVmaAllocator(),
        image,                                                  // pass reference of image that you want to create
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,                                       // image usage - tiling optimal because the renderer using staging buffer to copy pixel data
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,  // image properties - destination of buffer copy and sampled in the shader
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
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
        util::TransitionImageLayout(
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
        util::CopyBufferToImage(
            commandBuffer,
            stagingBuffer.buffer,
            image.image,
            SafeStaticCast<int, uint32>(texWidth),
            SafeStaticCast<int, uint32>(texHeight)
        );
        });
    commandQueue.push([&](VkCommandBuffer commandBuffer) {
        util::TransitionImageLayout(
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
    vmaDestroyBuffer(mkDeviceRef.GetVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}

void Texture::CreateTextureImageView()
{
    util::CreateImageView(
        mkDeviceRef.GetDevice(),
        image.image,
        imageView,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );
}

void Texture::CreateTextureSampler()
{
    // get device physical properties for limit of max anisotropy 
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(mkDeviceRef.GetPhysicalDevice(), &properties);

    // specify sampler creation info
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;                            // linear filtering in magnification
    samplerInfo.minFilter = VK_FILTER_LINEAR;                            // linear filtering in minification
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;           // repeat wrapping mode when texture going beyond the image dimensions                                                        
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;           // repeat wrapping mode
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;           // repeat wrapping mode
    samplerInfo.anisotropyEnable = VK_TRUE;                              // enable anisotropic filtering
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;  // max level of anisotropy
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;          // border color
    samplerInfo.unnormalizedCoordinates = VK_FALSE;                      // normalized u,v coordinates
    samplerInfo.compareEnable = VK_FALSE;                                // compare enable
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;                        // compare operation
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;              // mipmap mode
    samplerInfo.mipLodBias = 0.0f;                                       // mipmap level of detail bias
    samplerInfo.minLod = 0.0f;                                           // minimum level of detail
    samplerInfo.maxLod = 0.0f;                                           // maximum level of detail

    MK_CHECK(vkCreateSampler(mkDeviceRef.GetDevice(), &samplerInfo, nullptr, &sampler));
}