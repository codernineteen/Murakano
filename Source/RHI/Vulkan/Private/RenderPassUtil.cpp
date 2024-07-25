#include "RenderPassUtil.h"

namespace mk
{
	namespace vk
	{
		void CreateDefaultRenderPass(VkDevice device, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, VkRenderPass* renderPassPtr)
		{
			std::vector<VkAttachmentDescription> attachments(2);
			// Color attachment
			attachments[0].format = colorAttachmentFormat;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;

			// Depth attachment
			attachments[1].format = depthAttachmentFormat;
			attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;

			VkAttachmentReference colorAttachmentRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
			VkAttachmentReference depthAttachmentRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			std::vector<VkSubpassDependency> subpassDependencies(1);
			subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependencies[0].dstSubpass = 0;
			subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkSubpassDescription subpassDescription{};
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.colorAttachmentCount = 1;
			subpassDescription.pColorAttachments = &colorAttachmentRef;
			subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

			VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpassDescription;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
			renderPassInfo.pDependencies = subpassDependencies.data();

			MK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, renderPassPtr));
		}

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
				//depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
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
				subpass.colorAttachmentCount    = static_cast<uint32>(colorAttachmentRefs.size());
				subpass.pColorAttachments       = colorAttachmentRefs.data();
				subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : VK_NULL_HANDLE;

				/**
				* 1. srcSubpass - the index of the first subpass in the dependency
				* 2. dstSubpass - the index of the current subpass in the dependency
				* 3. srcStageMask - the pipeline stages that must be finished before the dependency can start
				* 4. dstStageMask - the pipeline stages that the dependency waits on
				* 5. srcAccessMask - the memory access used by srcSubpass
				* 6. dstAccessMask - the memory access used by dstSubpass
				*/
				VkSubpassDependency dependency{};
				dependency.srcSubpass    = (i == 0) ? VK_SUBPASS_EXTERNAL : (i - 1); // initial subpass starts from external
				dependency.dstSubpass    = i;				    
				dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.srcAccessMask = 0;
				dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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
}