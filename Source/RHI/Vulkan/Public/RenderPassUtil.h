#include <vulkan/vulkan.h>
#include <vector>
#include <assert.h>

#include "Types.h"
#include "Info.h"
#include "Macros.h"

namespace mkvk
{
	void CreateDefaultRenderPass(VkDevice device, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, VkRenderPass* renderPassPtr);
	VkRenderPass CreateRenderPass(
		VkDevice              device,
		std::vector<VkFormat> colorAttachmentFormats,
		VkFormat              depthFormat,
		/* parameters with default value */
		uint32                subpassCount = 1,
		bool                  clearColor = true,
		bool                  clearDepthStencil = true,
		VkImageLayout         initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout         finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // use different layout when the result is stored in the image or buffer
	);
}