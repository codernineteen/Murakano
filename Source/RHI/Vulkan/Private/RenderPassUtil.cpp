#include "RenderPassUtil.h"

namespace mkvk 
{
	VkRenderPass CreateRenderPass(
		VkDevice              device,
		std::vector<VkFormat> colorAttachmentFormats,
		VkFormat              depthAttachmentFormat,
		uint32                subpassCount,
		bool                  clearColor,
		bool                  clearDepthStencil,
		VkImageLayout         initialLayout,
		VkImageLayout         finalLayout
	) 
	{
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference>   colorAttachmentRefs;

		// create color attachments based on given formats
		for (const auto& format : colorAttachmentFormats)
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format         = format;
			colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp         = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : (
				(initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD
			);
			colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; // store result in memory
			colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout  = initialLayout;
			colorAttachment.finalLayout    = finalLayout;

			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = static_cast<uint32>(attachments.size()); // index of the attachment in the attachment descriptions array
			colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			attachments.push_back(colorAttachment);
			colorAttachmentRefs.push_back(colorAttachmentRef);
		}

		bool hasDepth = depthAttachmentFormat != VK_FORMAT_UNDEFINED;
		VkAttachmentReference depthAttachmentRef{};
		if (hasDepth)
		{
			VkAttachmentDescription depthAttachment{};
			/* depth spec */
			depthAttachment.format         = depthAttachmentFormat;
			depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp         = clearDepthStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; 
			/* stencil spec */
			depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			/* depth stencil ref */
			depthAttachmentRef.attachment  = static_cast<uint32>(attachments.size());
			depthAttachmentRef.layout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachments.push_back(depthAttachment);
		}

		// subpass - subsequent rendering operations that depend on the contents of the framebuffer
		std::vector<VkSubpassDescription> subpasses{};
		std::vector<VkSubpassDependency>  dependencies{};

		for (uint32 i = 0; i < subpassCount; i++)
		{
			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS; // subpass can be used for graphics or compute operations
			subpass.colorAttachmentCount    = 1;
			subpass.pColorAttachments       = colorAttachmentRefs.data();
			subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : VK_NULL_HANDLE;

			// specify indices of dependency and dependent subpass
			VkSubpassDependency dependency{};
			// only very first subpass starts from external
			dependency.srcSubpass    = (i == 0) ? VK_SUBPASS_EXTERNAL : i-1;
			dependency.dstSubpass    = i;				    
			dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			subpasses.push_back(subpass);
			dependencies.push_back(dependency);
		}

		// create render pass
		VkRenderPassCreateInfo renderPassInfo = vkinfo::GetRenderPassCreateInfo(attachments, subpasses, dependencies);
		VkRenderPass           renderPass;
		MK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

		return renderPass;
	}
}