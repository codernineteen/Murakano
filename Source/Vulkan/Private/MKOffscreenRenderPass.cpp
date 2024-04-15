#include "MKOffscreenRenderPass.h"

MKOffscreenRenderPass::MKOffscreenRenderPass(MKDevice* mkDevicePtr)
    :
        _mkDevicePtr(mkDevicePtr)
{
}

MKOffscreenRenderPass::~MKOffscreenRenderPass()
{
    vkDestroySampler(_mkDevicePtr->GetDevice(), _colorSampler, nullptr);
	vkDestroyImageView(_mkDevicePtr->GetDevice(), _colorImageView, nullptr);
	vmaDestroyImage(_mkDevicePtr->GetVmaAllocator(), _colorImage.image, _colorImage.allocation);

	vkDestroySampler(_mkDevicePtr->GetDevice(), _depthSampler, nullptr);
	vkDestroyImageView(_mkDevicePtr->GetDevice(), _depthImageView, nullptr);
	vmaDestroyImage(_mkDevicePtr->GetVmaAllocator(), _depthImage.image, _depthImage.allocation);

	vkDestroyFramebuffer(_mkDevicePtr->GetDevice(), _framebuffer, nullptr);
	vkDestroyRenderPass(_mkDevicePtr->GetDevice(), _renderPass, nullptr);
}

void MKOffscreenRenderPass::CreateOffscreenRenderPass(VkExtent2D extent)
{
    vmaDestroyImage(_mkDevicePtr->GetVmaAllocator(), _colorImage.image, _colorImage.allocation);
    vmaDestroyImage(_mkDevicePtr->GetVmaAllocator(), _depthImage.image, _depthImage.allocation);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;                            // linear filtering in magnification
    samplerInfo.minFilter = VK_FILTER_LINEAR;                            // linear filtering in minification
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;              // mipmap mode
    samplerInfo.mipLodBias = 0.0f;                                       // mipmap level of detail bias
    samplerInfo.minLod = 0.0f;                                           // minimum level of detail
    samplerInfo.maxLod = 0.0f;                                           // maximum level of detail
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;          // border color
    samplerInfo.unnormalizedCoordinates = VK_FALSE;                      // normalized u,v coordinates
    samplerInfo.compareEnable = VK_FALSE;                                // compare enable
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;                        // compare operation

    // color image
    util::CreateImage(
        _mkDevicePtr->GetVmaAllocator(),
        _colorImage,
        extent.width, extent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO,
        0
    );
    MK_CHECK(vkCreateSampler(_mkDevicePtr->GetDevice(), &samplerInfo, nullptr, &_colorSampler));


    VkImageViewCreateInfo colorImageViewInfo = {};
    colorImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageViewInfo.image = _colorImage.image;
    colorImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageViewInfo.subresourceRange.baseMipLevel = 0;
    colorImageViewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    colorImageViewInfo.subresourceRange.baseArrayLayer = 0;
    colorImageViewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    MK_CHECK(vkCreateImageView(_mkDevicePtr->GetDevice(), &colorImageViewInfo, nullptr, &_colorImageView));

    _colorDesc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    _colorDesc.imageView = _colorImageView;
    _colorDesc.sampler = _colorSampler;

    // depth
    util::CreateImage(
		_mkDevicePtr->GetVmaAllocator(),
		_depthImage,
		extent.width, extent.height,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0
	);

    MK_CHECK(vkCreateSampler(_mkDevicePtr->GetDevice(), &samplerInfo, nullptr, &_depthSampler));

    VkImageViewCreateInfo depthStencilView{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = VK_FORMAT_X8_D24_UNORM_PACK32;
    depthStencilView.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
    depthStencilView.image = _depthImage.image;
    MK_CHECK(vkCreateImageView(_mkDevicePtr->GetDevice(), &depthStencilView, nullptr, &_depthImageView));

    _depthDesc.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    _depthDesc.imageView = _depthImageView;
    _depthDesc.sampler = _depthSampler;


    // record command and submit
    VkCommandPool cmdPool;
    GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT); // transient command pool for one-time command buffer
    VkCommandBuffer commandBuffer;
    GCommandService->BeginSingleTimeCommands(commandBuffer, cmdPool);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.baseArrayLayer = 0;
    util::TransitionImageLayout(
        commandBuffer,
        _colorImage.image,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange
    );

    subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    util::TransitionImageLayout(
        commandBuffer,
        _depthImage.image,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        subresourceRange
    );
    GCommandService->EndSingleTimeCommands(commandBuffer, cmdPool);

    // render pass
    if (_renderPass != VK_NULL_HANDLE)
    {
        std::vector<VkAttachmentDescription> allAttachments;
        std::vector<VkAttachmentReference>   colorAttachmentRefs;

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        allAttachments.push_back(colorAttachment);
        colorAttachmentRefs.push_back(colorAttachmentRef);

        VkAttachmentReference depthAttachmentRef = {};
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = VK_FORMAT_X8_D24_UNORM_PACK32;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        allAttachments.push_back(depthAttachment);

        std::vector<VkSubpassDescription> subpasses;
        std::vector<VkSubpassDependency>  subpassDependencies;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        subpasses.push_back(subpass);
        subpassDependencies.push_back(dependency);

        VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
        renderPassInfo.pAttachments = allAttachments.data();
        renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
        renderPassInfo.pDependencies = subpassDependencies.data();
        MK_CHECK(vkCreateRenderPass(_mkDevicePtr->GetDevice(), &renderPassInfo, nullptr, &_renderPass));
	}

    // frame buffer for offscreen
    std::vector<VkImageView> attachments = { _colorDesc.imageView, _depthDesc.imageView };
    VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    framebufferInfo.renderPass = _renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    MK_CHECK(vkCreateFramebuffer(_mkDevicePtr->GetDevice(), &framebufferInfo, nullptr, &_framebuffer));
}
