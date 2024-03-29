#include "MKOffScreenRender.h"
#include "MKOffscreenRender.h"

MKOffscreenRender::MKOffscreenRender(MKDevice& mkDeviceRef)
	: _mkDeviceRef(mkDeviceRef)
{
}

MKOffscreenRender::~MKOffscreenRender()
{
	DestroyOffscreenRenderResource();
	vkDestroyRenderPass(_mkDeviceRef.GetDevice(), _vkOffscreenRenderPass, nullptr);
}

void MKOffscreenRender::CreateOffscreenRenderResource(VkExtent2D extent)
{
	// create color image
	util::CreateImage(
		_mkDeviceRef.GetVmaAllocator(),
		_vkOffscreenColor,
		extent.width,
		extent.height,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = _vkOffscreenColor.image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	vkCreateImageView(_mkDeviceRef.GetDevice(), &imageViewCreateInfo, nullptr, &_vkOffscreenColorDescriptor.imageView);
	vkCreateSampler(_mkDeviceRef.GetDevice(), &samplerInfo, nullptr, &_vkOffscreenColorDescriptor.sampler);
	_vkOffscreenColorDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	// create depth buffer
	util::CreateImage(
		_mkDeviceRef.GetVmaAllocator(),
		_vkOffscreenDepth,
		extent.width,
		extent.height,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	);
	
	VkImageViewCreateInfo depthViewCreateInfo{};
	depthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthViewCreateInfo.image = _vkOffscreenDepth.image;
	depthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthViewCreateInfo.format = VK_FORMAT_X8_D24_UNORM_PACK32;
	depthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthViewCreateInfo.subresourceRange.baseMipLevel = 0;
	depthViewCreateInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	depthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	depthViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCreateImageView(_mkDeviceRef.GetDevice(), &depthViewCreateInfo, nullptr, &_vkOffscreenDepthDescriptor.imageView);
	_vkOffscreenDepthDescriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkCommandPool cmdPool;
	GCommandService->CreateCommandPool(&cmdPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	VkCommandBuffer cmdBuffer;
	GCommandService->BeginSingleTimeCommands(cmdBuffer, cmdPool);

	VkImageSubresourceRange subresouceRange{};
	subresouceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresouceRange.baseMipLevel = 0;
	subresouceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	subresouceRange.baseArrayLayer = 0;
	subresouceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	util::TransitionImageLayout(cmdBuffer, _vkOffscreenColor.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, subresouceRange);
	subresouceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	util::TransitionImageLayout(cmdBuffer, _vkOffscreenDepth.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, subresouceRange);

	GCommandService->EndSingleTimeCommands(cmdBuffer, cmdPool);

	/**
	* create render pass
	*/
	if (_vkOffscreenRenderPass == VK_NULL_HANDLE)
	{
		std::vector<VkAttachmentDescription> allAttachments;
		std::vector<VkAttachmentReference>   colorAttachmentRefs;

		// 1. specify color attachment
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

		// 2. specify depth attachment
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = VK_FORMAT_X8_D24_UNORM_PACK32;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = static_cast<uint32_t>(allAttachments.size());
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		allAttachments.push_back(depthAttachment);

		// 3. specify subpass
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

		// 4. create actual render pass
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
		renderPassCreateInfo.pAttachments = allAttachments.data();
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
		renderPassCreateInfo.pSubpasses = subpasses.data();
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassCreateInfo.pDependencies = subpassDependencies.data();
		MK_CHECK(vkCreateRenderPass(_mkDeviceRef.GetDevice(), &renderPassCreateInfo, nullptr, &_vkOffscreenRenderPass));
	}

	// create framebuffer
	std::vector<VkImageView> attachments = { _vkOffscreenColorDescriptor.imageView, _vkOffscreenDepthDescriptor.imageView };
	VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	info.renderPass = _vkOffscreenRenderPass;
	info.attachmentCount = 2;
	info.pAttachments = attachments.data();
	info.width = extent.width;
	info.height = extent.height;
	info.layers = 1;
	vkCreateFramebuffer(_mkDeviceRef.GetDevice(), &info, nullptr, &_vkOffscreenFrameBuffer);
}

void MKOffscreenRender::DestroyOffscreenRenderResource() 
{
	// destory previous resources first for the recreation case.
	vmaDestroyImage(_mkDeviceRef.GetVmaAllocator(), _vkOffscreenColor.image, _vkOffscreenColor.allocation);
	vmaDestroyImage(_mkDeviceRef.GetVmaAllocator(), _vkOffscreenDepth.image, _vkOffscreenDepth.allocation);
	vkDestroyImageView(_mkDeviceRef.GetDevice(), _vkOffscreenColorDescriptor.imageView, nullptr);
	vkDestroyImageView(_mkDeviceRef.GetDevice(), _vkOffscreenDepthDescriptor.imageView, nullptr);
	vkDestroySampler(_mkDeviceRef.GetDevice(), _vkOffscreenColorDescriptor.sampler, nullptr);
	vkDestroyFramebuffer(_mkDeviceRef.GetDevice(), _vkOffscreenFrameBuffer, nullptr); // destroy old framebuffer
}

void MKOffscreenRender::RecreateOffscreenRenderResource(VkExtent2D extent) 
{
	DestroyOffscreenRenderResource();
	CreateOffscreenRenderResource(extent);
}