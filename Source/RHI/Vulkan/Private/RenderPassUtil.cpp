#include "RenderPassUtil.h"

namespace mkvk 
{
	VkRenderPass CreateDefaultRenderPass(VkDevice device, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format         = colorAttachmentFormat;
		colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;				// TODO : multisampling
		colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;			// clear framebuffer to black before drawing a new frame
		colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;			// store rendered contents in memory
		colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// TODO : for depth testing , use VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format         = depthAttachmentFormat;
		depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // depth image won't be used after drawing has finished
		depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;   // ignore previous depth data
		depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthAttachmentRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };

		// subpass
		//  - subsequent rendering operations that depend on the contents of the framebuffer
		std::vector<VkSubpassDescription> subpasses{};
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS; // subpass can be used for graphics or compute operations
		subpass.colorAttachmentCount    = 1;
		subpass.pColorAttachments       = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpasses.push_back(subpass);

		/**
		* 1. srcSubpass - the index of the first subpass in the dependency
		* 2. dstSubpass - the index of the current subpass in the dependency
		* 3. srcStageMask - the pipeline stages that must be finished before the dependency can start
		* 4. dstStageMask - the pipeline stages that the dependency waits on
		* 5. srcAccessMask - the memory access used by srcSubpass
		* 6. dstAccessMask - the memory access used by dstSubpass
		*/
		std::vector<VkSubpassDependency> dependencies(2);
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_NONE_KHR;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// create render pass
		VkRenderPassCreateInfo renderPassInfo = vkinfo::GetRenderPassCreateInfo(attachments, subpasses, dependencies);
		VkRenderPass renderPass;
		MK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

		return renderPass;
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