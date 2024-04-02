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

	/**
	* 1. srcSubpass - the index of the first subpass in the dependency
	* 2. dstSubpass - the index of the current subpass in the dependency
	* 3. srcStageMask - the pipeline stages that must be finished before the dependency can start
	* 4. dstStageMask - the pipeline stages that the dependency waits on
	* 5. srcAccessMask - the memory access used by srcSubpass
	* 6. dstAccessMask - the memory access used by dstSubpass
	*/
	std::array<VkSubpassDependency, 2> dependencies;
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
	VkRenderPassCreateInfo renderPassInfo = vkinfo::GetRenderPassCreateInfo(attachments, subpass, static_cast<uint32>(dependencies.size()), dependencies.data());
	MK_CHECK(vkCreateRenderPass(_mkDeviceRef.GetDevice(), &renderPassInfo, nullptr, &_vkRenderPass));
}

MKRenderPass::~MKRenderPass()
{
	vkDestroyRenderPass(_mkDeviceRef.GetDevice(), _vkRenderPass, nullptr);

#ifndef NDEBUG
	MK_LOG("render pass destroyed");
#endif
}
