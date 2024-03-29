#include "MKRenderPass.h"

MKRenderPass::MKRenderPass(const MKDevice& mkDeviceRef, VkFormat swapchainImageFormat)
	: _mkDeviceRef(mkDeviceRef)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;				// TODO : multisampling
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;			// clear framebuffer to black before drawing a new frame
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			// store rendered contents in memory
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// TODO : for depth testing , use VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL

	//VkAttachmentDescription colorAttachmentResolve{};
	//colorAttachmentResolve.format = _mkSwapchainRef.GetSwapchainImageFormat();
	//colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT; // no multisampling for presentation.
	//colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = util::FindDepthFormat(_mkDeviceRef.GetPhysicalDevice());
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // depth image won't be used after drawing has finished
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;   // ignore previous depth data
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // index of the attachment in the attachment descriptions array
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1; // set an index next to color attachment
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/*VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;*/

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	// subpass
	//  - subsequent rendering operations that depend on the contents of the framebuffer
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // subpass can be used for graphics or compute operations
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	//subpass.pResolveAttachments = &colorAttachmentResolveRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	// specify indices of dependency and dependent subpass
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass before or after the render pass
	dependency.dstSubpass = 0;				     // our first subpass
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	// create render pass
	VkRenderPassCreateInfo renderPassInfo = vkinfo::GetRenderPassCreateInfo(attachments, subpass, dependency);
	MK_CHECK(vkCreateRenderPass(_mkDeviceRef.GetDevice(), &renderPassInfo, nullptr, &_vkRenderPass));
}

MKRenderPass::~MKRenderPass()
{
	vkDestroyRenderPass(_mkDeviceRef.GetDevice(), _vkRenderPass, nullptr);

#ifndef NDEBUG
	MK_LOG("render pass destroyed");
#endif
}
