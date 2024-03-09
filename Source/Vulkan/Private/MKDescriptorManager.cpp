#include "MKDescriptorManager.h"

/**
* ---------- public ----------
*/

MKDescriptorManager::MKDescriptorManager()
{
}

MKDescriptorManager::~MKDescriptorManager()
{
    // destroy descriptor pools
    for(auto& pool : _vkDescriptorPoolReady)
        vkDestroyDescriptorPool(_mkDevicePtr->GetDevice(), pool, nullptr);

    for (auto& pool : _vkDescriptorPoolFull)
        vkDestroyDescriptorPool(_mkDevicePtr->GetDevice(), pool, nullptr);

#ifndef NDEBUG
    MK_LOG("combined image sampler destroyed");
    MK_LOG("texture image view destroyed");
    MK_LOG("texture image destroyed and freed its memory");
    MK_LOG("uniform buffer objects destroyed");
    MK_LOG("descriptor pool destroyed");
    MK_LOG("descriptor set layout destroyed");
#endif
}

void MKDescriptorManager::InitDescriptorManager(MKDevice* mkDevicePtr)
{
    // initialize device pointer and swapchain extent
	_mkDevicePtr = mkDevicePtr;
}


void MKDescriptorManager::AllocateDescriptorSet(std::vector<VkDescriptorSet>& descriptorSets, VkDescriptorSetLayout layout)
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
    VkDescriptorPool poolInUse = GetDescriptorPool();

    // specify a single descriptor set allocation info
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = poolInUse;
    allocInfo.descriptorSetCount = static_cast<uint32>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    allocInfo.pNext = nullptr;
    
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VkResult res = vkAllocateDescriptorSets(_mkDevicePtr->GetDevice(), &allocInfo, descriptorSets.data());
    
    if (res != VK_SUCCESS)
    {
        _vkDescriptorPoolFull.push_back(poolInUse); // push back the full descriptor pool

        poolInUse = GetDescriptorPool(); // get new descriptor pool
        allocInfo.descriptorPool = poolInUse; // assign new descriptor pool to allocation info

        MK_CHECK(vkAllocateDescriptorSets(_mkDevicePtr->GetDevice(), &allocInfo, descriptorSets.data()));
    }

    _vkDescriptorPoolReady.push_back(poolInUse); // push back the in-use descriptor pool
}

VkDescriptorPool MKDescriptorManager::GetDescriptorPool()
{
	VkDescriptorPool newPool = VK_NULL_HANDLE;

    if (!_vkDescriptorPoolReady.empty())
    {
        newPool = _vkDescriptorPoolReady.back();
        _vkDescriptorPoolReady.pop_back();
    }
    else
    {
        newPool = CreateDescriptorPool(_setsPerPool); // create new descriptor pool and assign it to in-use pool

        if (_setsPerPool < 1024)
            _setsPerPool = _setsPerPool * 1.5; // gradually increase the number of sets per pool
        else
            _setsPerPool = 1024; // if it exceeds 1024, fix it to 1024
    }

    return newPool;
}

VkDescriptorPool MKDescriptorManager::CreateDescriptorPool(uint32 setCount)
{
    for (auto& poolSize : _vkDescriptorPoolSizes)
        poolSize.descriptorCount = poolSize.descriptorCount * setCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = SafeStaticCast<size_t, uint32>(_vkDescriptorPoolSizes.size());
    poolInfo.pPoolSizes = _vkDescriptorPoolSizes.data();

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    MK_CHECK(vkCreateDescriptorPool(_mkDevicePtr->GetDevice(), &poolInfo, nullptr, &descriptorPool));

    return descriptorPool;
}

void MKDescriptorManager::ResetDescriptorPool()
{
    for(auto pool : _vkDescriptorPoolReady)
        vkResetDescriptorPool(_mkDevicePtr->GetDevice(), pool, 0);

    for (auto pool : _vkDescriptorPoolReady)
    {
        vkResetDescriptorPool(_mkDevicePtr->GetDevice(), pool, 0);
        _vkDescriptorPoolReady.push_back(pool);
    }

    _vkDescriptorPoolFull.clear();
}

void MKDescriptorManager::AddDescriptorSetLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags shaderStageFlags, uint32_t binding, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding = binding;                 // binding index in shader.
    newBinding.descriptorType = descriptorType;   // type of descriptor
    newBinding.descriptorCount = descriptorCount; // number of descriptors
    newBinding.stageFlags = shaderStageFlags;     // shader stage flags
    newBinding.pImmutableSamplers = nullptr;

    _vkWaitingBindings.push_back(newBinding);
}

void MKDescriptorManager::CreateDescriptorSetLayout(VkDescriptorSetLayout& layout)
{
    // specify descriptor set layout creation info
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = SafeStaticCast<size_t, uint32>(_vkWaitingBindings.size());
    layoutInfo.pBindings = _vkWaitingBindings.data();

    // create descriptor set layout
    MK_CHECK(vkCreateDescriptorSetLayout(_mkDevicePtr->GetDevice(), &layoutInfo, nullptr, &layout));

    // be sure to clear layout bindings after creating descriptor set layout !!
    _vkWaitingBindings.clear();
}

void MKDescriptorManager::WriteBufferToDescriptorSet(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32 dstBinding, VkDescriptorType descriptorType)
{
    // To keep memory of buffer info, store it in the deque temporarily
    std::shared_ptr<VkDescriptorBufferInfo> bufferInfo = std::make_shared<VkDescriptorBufferInfo>();
    bufferInfo->buffer = buffer;
    bufferInfo->offset = offset;
    bufferInfo->range = range;
    _vkWaitingBufferInfos.insert(bufferInfo);

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = VK_NULL_HANDLE; // specify dst set when update descriptor set
	descriptorWrite.dstBinding = dstBinding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = descriptorType;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = bufferInfo.get();

	_vkWaitingWrites.push_back(descriptorWrite);
}

void MKDescriptorManager::WriteImageToDescriptorSet(VkImageView imageView, VkSampler imageSampler, VkImageLayout imageLayout, uint32 dstBinding, VkDescriptorType descriptorType)
{
    // To keep memory of image info, store it in the deque temporarily
    std::shared_ptr<VkDescriptorImageInfo> imageInfo = std::make_shared<VkDescriptorImageInfo>();
    imageInfo->imageView = imageView;
    imageInfo->sampler = imageSampler;
    imageInfo->imageLayout = imageLayout;
    _vkWaitingImageInfos.insert(imageInfo);

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = VK_NULL_HANDLE; // specify dst set when update descriptor set
	descriptorWrite.dstBinding = dstBinding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = descriptorType;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = imageInfo.get();

	_vkWaitingWrites.push_back(descriptorWrite);
}

void MKDescriptorManager::UpdateDescriptorSet(VkDescriptorSet descriptorSet)
{
    for(auto& write : _vkWaitingWrites)
		write.dstSet = descriptorSet;

    // update descriptor sets with waiting writes.
    vkUpdateDescriptorSets(
		_mkDevicePtr->GetDevice(),
		SafeStaticCast<uint32, size_t>(_vkWaitingWrites.size()),
		_vkWaitingWrites.data(),
		0,
		nullptr
	);

    // clear waiting writes and related buffer/image infos after updating descriptor set
    _vkWaitingBufferInfos.clear();
    _vkWaitingImageInfos.clear();
	_vkWaitingWrites.clear();
}

void MKDescriptorManager::CreateTextureImage(const std::string texturePath, VkImageAllocated& textureImage) 
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = SafeStaticCast<int, VkDeviceSize>(texWidth * texHeight * 4);

    if (!pixels) 
        throw std::runtime_error("failed to load texture image!");

    // same manner as vertex buffer creation
    VkBufferAllocated stagingBuffer = util::CreateBuffer(
		_mkDevicePtr->GetVmaAllocator(),
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
        _mkDevicePtr->GetVmaAllocator(),
        textureImage,                                                  // pass reference of image that you want to create
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
    std::queue<VoidLambda> commandQueue;
    commandQueue.push([&](VkCommandBuffer commandBuffer) {
        TransitionImageLayout(
            commandBuffer,                            // command buffer
            textureImage.image,                       // texture image
            VK_FORMAT_R8G8B8A8_SRGB,                  // format
            VK_IMAGE_LAYOUT_UNDEFINED,                // old layout
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL      // new layout
        );
    });
    commandQueue.push([&](VkCommandBuffer commandBuffer) {
        // copy image data from staging buffer to texture image after layout transition
        CopyBufferToImage(
            commandBuffer,
            stagingBuffer.buffer,
            textureImage.image,
            SafeStaticCast<int, uint32>(texWidth),
            SafeStaticCast<int, uint32>(texHeight)
        );
    });
    commandQueue.push([&](VkCommandBuffer commandBuffer) {
        TransitionImageLayout(
            commandBuffer,                            // command buffer
            textureImage.image,                       // texture image
            VK_FORMAT_R8G8B8A8_SRGB,                  // format
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // old layout
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // new layout
        );
    });

    GCommandService->AsyncExecuteCommands(commandQueue);


    // destroy staging buffer and free memory
    vmaDestroyBuffer(_mkDevicePtr->GetVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}

void MKDescriptorManager::CreateTextureImageView(VkImage& textureImage, VkImageView& textureImageView) 
{
    util::CreateImageView(
        _mkDevicePtr->GetDevice(),
        textureImage,
        textureImageView,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );
}

void MKDescriptorManager::CreateTextureSampler(VkSampler& textureSampler)
{
    // get device physical properties for limit of max anisotropy 
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(_mkDevicePtr->GetPhysicalDevice(), &properties);

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

    MK_CHECK(vkCreateSampler(_mkDevicePtr->GetDevice(), &samplerInfo, nullptr, &textureSampler));
}

void MKDescriptorManager::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) 
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
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // read - EARLY_FRAGMENT_TESTS_BIT, write - LATE_FRAGMENT_TESTS_BIT
    }
    else
        throw std::invalid_argument("unsupported layout transition!");

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // no tranfer on any queue, so ignored
    barrier.image = image;                                           // specify image to transition layout

    // set proper aspect mask based on the layout
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if(HasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
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
}

void MKDescriptorManager::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
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
}