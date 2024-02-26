#include "MKDescriptorManager.h"

MKDescriptorManager::MKDescriptorManager(MKDevice& mkDeviceRef, const VkExtent2D& swapchainExtent)
    : _mkDeviceRef(mkDeviceRef), _vkSwapchainExtent(swapchainExtent)
{

    /**
    * Create Descriptor resources
    * 1. uniform buffer object
    * 2. texture sampler
    */
    CreateUniformBuffer();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();

    /**
    * Descriptor set layout binding
    *  1. uniform buffer object layout binding
    *  2. texture sampler layout binding
    * Every binding to the set needs to be described through VkDescriptorSetLayoutBinding before creating VkDescriptorSetLayout.
    */

    // uniform buffer object layout binding
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;                                         // layout(binding = 0)
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  
	uboLayoutBinding.descriptorCount = 1;                                 // model, view, projection transformation is a single uniform buffer object.
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;


    // texture sampler layout binding
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // combine layout bindings into single descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = SafeStaticCast<size_t, uint32>(bindings.size());
	layoutInfo.pBindings = bindings.data();

    // create descriptor set layout
    if (vkCreateDescriptorSetLayout(_mkDeviceRef.GetDevice(), &layoutInfo, nullptr, &_vkDescriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");

    /**
    * Create Descriptor Pool
    *  1. specify descriptor pool size
    *  2. specify descriptor pool creation info
    * Descriptor pool size should be matched with size of descriptor set layout bindings.
    */
    std::array<VkDescriptorPoolSize,2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                                         // descriptor type for uniform buffer object
    poolSizes[0].descriptorCount = SafeStaticCast<int, uint32>(MAX_FRAMES_IN_FLIGHT);         // descriptor count for each frame in flight
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = SafeStaticCast<size_t, uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = SafeStaticCast<int, uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if(vkCreateDescriptorPool(_mkDeviceRef.GetDevice(), &poolInfo, nullptr, &_vkDescriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");

    // allocate descriptor set
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _vkDescriptorSetLayout);  // same layout for each frame in flight
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _vkDescriptorPool;
    allocInfo.descriptorSetCount = SafeStaticCast<int, uint32>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    _vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    // create descriptor sets for each frame in flight
    if(vkAllocateDescriptorSets(_mkDeviceRef.GetDevice(), &allocInfo, _vkDescriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate descriptor set!");
    
    // populate descriptor set with actual buffer
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // buffer descriptor
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = _vkUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

        // sampler descriptor
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = _vkTextureImageView;
        imageInfo.sampler = _vkTextureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
        // ubo descriptor write
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = _vkDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;             // binding = 0
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;  // assign buffer info
        // texture sampler descriptor write
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _vkDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;             // binding = 1
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;    // assing image info

        // update descriptor set with specified VkWriteDescriptorSet
		vkUpdateDescriptorSets(
            _mkDeviceRef.GetDevice(), 
            SafeStaticCast<size_t, uint32>(descriptorWrites.size()), 
            descriptorWrites.data(), 
            0, 
            nullptr
        );
	}
}

MKDescriptorManager::~MKDescriptorManager()
{
    // cleanup texture resources
    vkDestroySampler(_mkDeviceRef.GetDevice(), _vkTextureSampler, nullptr);
    vkDestroyImageView(_mkDeviceRef.GetDevice(), _vkTextureImageView, nullptr);
    vkDestroyImage(_mkDeviceRef.GetDevice(), _vkTextureImage, nullptr);
    vkFreeMemory(_mkDeviceRef.GetDevice(), _vkTextureImageMemory, nullptr);

    // cleanup uniform buffer
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(_mkDeviceRef.GetDevice(), _vkUniformBuffers[i], nullptr);
        vkFreeMemory(_mkDeviceRef.GetDevice(), _vkUniformBuffersMemory[i], nullptr);
    }

    // destroy descriptor pool
    vkDestroyDescriptorPool(_mkDeviceRef.GetDevice(), _vkDescriptorPool, nullptr);

    // destroy descriptor set layout
    vkDestroyDescriptorSetLayout(_mkDeviceRef.GetDevice(), _vkDescriptorSetLayout, nullptr);
}

void MKDescriptorManager::UpdateUniformBuffer(uint32 currentFrame)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};

    // Apply model transformation
    auto modelMat = dx::XMMatrixRotationAxis(dx::XMVECTOR{ 0.0f, 0.0f, 1.0f }, time * dx::XMConvertToRadians(90.0f));
    modelMat = dx::XMMatrixTranspose(modelMat);

    // Transform into view space
    auto viewMat = dx::XMMatrixLookAtLH(dx::XMVECTOR{ 2.0f, 2.0f, 2.0f }, dx::XMVECTOR{ 0.0f, 0.0f, 0.0f }, dx::XMVECTOR{ 0.0f, 0.0f, -1.0f });
    viewMat = dx::XMMatrixTranspose(viewMat);
    /**
    * Perspective projection
    * 1. fovy : 45 degree field of view
    * 2. aspect ratio : swapchain extent width / swapchain extent height
    * 3. near plane : 0.1f
    * 4. far plane : 10.0f
    */
    auto projectionMat = dx::XMMatrixPerspectiveFovLH(
        dx::XMConvertToRadians(45.0f), 
        _vkSwapchainExtent.width / SafeStaticCast<uint32, float>(_vkSwapchainExtent.height), 
        0.1f, 
        10.0f
    );
    projectionMat = dx::XMMatrixTranspose(projectionMat);

    // Because SIMD operation is supported, i did multiplication in application side, not in shader side.
    ubo.mvpMat = projectionMat * viewMat * modelMat;

    memcpy(_vkUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}


/**
* ---------- private ----------
*/
void MKDescriptorManager::CreateUniformBuffer()
{
    // create uniform buffer object
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    _vkUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _vkUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    _vkUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        util::CreateBuffer(
            _mkDeviceRef.GetPhysicalDevice(),
            _mkDeviceRef.GetDevice(), bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _vkUniformBuffers[i],
            _vkUniformBuffersMemory[i]
        );

        // persistent mapping without unmapping memory.
        vkMapMemory(_mkDeviceRef.GetDevice(), _vkUniformBuffersMemory[i], 0, bufferSize, 0, &_vkUniformBuffersMapped[i]);
    }
}

void MKDescriptorManager::CreateTextureImage() 
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../../../resources/textures/wood.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = SafeStaticCast<int, VkDeviceSize>(texWidth * texHeight * 4);

    if (!pixels) 
        throw std::runtime_error("failed to load texture image!");

    // same manner as vertex buffer creation
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    util::CreateBuffer(
		_mkDeviceRef.GetPhysicalDevice(), 
		_mkDeviceRef.GetDevice(), 
		imageSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, 
		stagingBufferMemory
	);

    // map the buffer memory to copy the pixel data into it
    void* data;
    vkMapMemory(_mkDeviceRef.GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(_mkDeviceRef.GetDevice(), stagingBufferMemory);

    // free after copying pixel data to staging buffer
    stbi_image_free(pixels);
   
    util::CreateImage(
        _mkDeviceRef.GetPhysicalDevice(),
        _mkDeviceRef.GetDevice(),
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,                                       // image usage - tiling optimal because the renderer using staging buffer to copy pixel data
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,  // image properties - destination of buffer copy and sampled in the shader
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _vkTextureImage,
        _vkTextureImageMemory
    );

    /**
    * layout transitions
    *  1. undefined -> transfer destination (initial layout transfer writes, no need to wait for anything)
    *  2. transfer destination -> shader reading (need to wait 'transfer writes')
    */
    TransitionImageLayout(
        _vkTextureImage,                          // texture image
        VK_FORMAT_R8G8B8A8_SRGB,                  // format
        VK_IMAGE_LAYOUT_UNDEFINED,                // old layout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL      // new layout
    );
    // copy image data from staging buffer to texture image after layout transition
    CopyBufferToImage(
        stagingBuffer, 
        _vkTextureImage, 
        SafeStaticCast<int, uint32>(texWidth), 
        SafeStaticCast<int, uint32>(texHeight)
    );
    TransitionImageLayout(
        _vkTextureImage,                          // texture image
        VK_FORMAT_R8G8B8A8_SRGB,                  // format
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // old layout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // new layout
    );

    // destroy staging buffer and free memory
    vkDestroyBuffer(_mkDeviceRef.GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(_mkDeviceRef.GetDevice(), stagingBufferMemory, nullptr);
}

void MKDescriptorManager::CreateTextureImageView() 
{
    util::CreateImageView(
        _mkDeviceRef.GetDevice(),
        _vkTextureImage,
        _vkTextureImageView,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );
}

void MKDescriptorManager::CreateTextureSampler()
{
    // get device physical properties for limit of max anisotropy 
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(_mkDeviceRef.GetPhysicalDevice(), &properties);

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

	if (vkCreateSampler(_mkDeviceRef.GetDevice(), &samplerInfo, nullptr, &_vkTextureSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");

}

void MKDescriptorManager::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) 
{
    /**
    * vkCmdPipelineBarrier specification
    * 1. command buffer
    * 2. source stage mask - pipeline stage at which the operations occur that should happen before the barrier
    * 3. destination stage mask - pipeline stage at which the operations occur that should happen after the barrier
    * 4. dependency flags - specify how to handle the barrier in terms of memory dependencies
    * 5. memory barrier count, memory barriers - reference to memory barriers
    * 6. buffer memory barrier count, buffer memory barriers - reference to buffer memory barriers
    * 7. image memory barrier count, image memory barriers - reference to image memory barriers
    */
    GCommandService->ExecuteSingleTimeCommands([&](VkCommandBuffer commandBuffer) {
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // set src access mask and dst access mask based on a kind of layout transitions
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        {
            barrier.srcAccessMask = 0;                             // no need to wait for anything
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // transfer write
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;       // for operations beyond barrier
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // transfer write
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;     // shader read
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
            throw std::invalid_argument("unsupported layout transition!");

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
        barrier.image = image;                                           // specify image to transition layout
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });
}

void MKDescriptorManager::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    GCommandService->ExecuteSingleTimeCommands([&](VkCommandBuffer commandBuffer) {
        VkBufferImageCopy region{};
        // which part of buffer to copy
        region.bufferOffset = 0;
        region.bufferRowLength = 0;   // specify how the pixels are laid out in memory(1)
        region.bufferImageHeight = 0; // specify how the pixels are laid out in memory(2)

        // to which part of image
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width, 
            height,
            1 // depth 1 because texture image is 2D
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // specify the layout of the image assuming image has already been transitioned to the layout
			1,
			&region
		);
    });
}