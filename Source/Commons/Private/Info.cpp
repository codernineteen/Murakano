#include "Info.h"

namespace vkinfo
{
	VkApplicationInfo GetApplicationInfo()
	{
		// specify application create info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // set app info enum variant to struct Type field
		appInfo.pApplicationName = "Murakano"; // application name
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // version specifier
		appInfo.pEngineName = "NO ENGINE";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3; // Murakano using v1.3.268 vulkan api.

		return appInfo;
	}

	VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;

		return createInfo;
	}

	VkInstanceCreateInfo GetInstanceCreateInfo(VkApplicationInfo* appInfo, size_t extensionSize, const char* const* extensionNames, VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo)
	{
		VkInstanceCreateInfo instanceInfo{};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = appInfo;
		instanceInfo.enabledExtensionCount = SafeStaticCast<size_t, uint32>(extensionSize);
		instanceInfo.ppEnabledExtensionNames = extensionNames;
#ifdef NDEBUG
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.pNext = nullptr;
#else
		instanceInfo.enabledLayerCount = SafeStaticCast<size_t, uint32>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
		instanceInfo.pNext = debugCreateInfo;
#endif

		return instanceInfo;
	}

	VkDeviceCreateInfo GetDeviceCreateInfo(const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos, VkPhysicalDeviceFeatures2& deviceFeatures2, const std::vector<const char*>& deviceExtensions)
	{
		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.queueCreateInfoCount = SafeStaticCast<size_t, uint32>(queueCreateInfos.size());
		deviceCreateInfo.enabledExtensionCount = SafeStaticCast<size_t, uint32>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
#ifdef NDEBUG
		deviceCreateInfo.enabledLayerCount = 0;													        // number of layers
		deviceCreateInfo.ppEnabledLayerNames = nullptr;											        // layers
#else
		deviceCreateInfo.enabledLayerCount = SafeStaticCast<size_t, uint32>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#endif
		deviceCreateInfo.pNext = &deviceFeatures2;													    // pointer to the device features 2

		return deviceCreateInfo;
	}

	VkDeviceQueueCreateInfo GetDeviceQueueCreateInfo(uint32 queueFamilyIndex)
	{
		float queuePriority = 1.0f; // priority of the queue
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex; // index of the queue family to create
		queueCreateInfo.queueCount = 1;						 // number of queues to create
		queueCreateInfo.pQueuePriorities = &queuePriority;	 // array of queue priorities

		return queueCreateInfo;
	}

	VkSwapchainCreateInfoKHR GetSwapchainCreateInfo(VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat, VkSurfaceCapabilitiesKHR capabilities, VkPresentModeKHR presentMode, uint32 imageCount, VkExtent2D extent, uint32 queueFamilyIndices[], bool isExclusive)
	{
		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = extent;
		swapchainCreateInfo.imageArrayLayers = 1; // always 1 except for stereoscopic 3D application
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // render directly to images
		if (!isExclusive) 
		{
			// If graphics family and present family are separate queue, swapchain image shared by multiple queue families
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.queueFamilyIndexCount = 0;
			swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}
		swapchainCreateInfo.preTransform = capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE; // ignore the pixels that are obscured by other windows
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	
		return swapchainCreateInfo;
	}

	VkFramebufferCreateInfo GetFramebufferCreateInfo(VkRenderPass renderPass, const std::array<VkImageView, 2>& attachments, VkExtent2D extent)
	{
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = SafeStaticCast<size_t, uint32>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1; // number of layers in image arrays

		return framebufferInfo;
	}

	VkRenderPassCreateInfo GetRenderPassCreateInfo(const std::array<VkAttachmentDescription, 2>& attachments, const VkSubpassDescription& subpass, uint32 depCount, const VkSubpassDependency* pDependencies)
	{
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = SafeStaticCast<size_t, uint32>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = depCount;
		renderPassInfo.pDependencies = pDependencies;

		return renderPassInfo;
	}

	VkPipelineShaderStageCreateInfo GetPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const std::string& entryPoint)
	{
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = stage;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = entryPoint.c_str();
		shaderStageInfo.pSpecializationInfo = nullptr; // compiler optimization purpose

		return shaderStageInfo;
	}

	VkPipelineInputAssemblyStateCreateInfo GetPipelineInputAssemblyStateCreateInfo()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;	// keep the triangle list topology without restarting the assembly

		return inputAssembly;
	}

	VkPipelineViewportStateCreateInfo GetPipelineViewportStateCreateInfo() 
	{
		VkPipelineViewportStateCreateInfo viewportState{};	// viewport and scissor rectangle
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;					// number of viewports
		viewportState.scissorCount = 1;						// number of scissor rectangles

		return viewportState;
	}
	
	VkPipelineDynamicStateCreateInfo GetPipelineDynamicStateCreateInfo(const std::vector<VkDynamicState>& dynamicStates)
	{
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = SafeStaticCast<size_t, uint32>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		return dynamicState;
	}

	VkPipelineRasterizationStateCreateInfo GetPipelineRasterizationStateCreateInfo()
	{
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;					// if true, fragments beyond near and far planes are clamped instead of discarded
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;							// thickness of lines in terms of number of fragments
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;			// back face culling setting
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // vertex order for faces to be considered front-facing
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;				// Optional
		rasterizer.depthBiasClamp = 0.0f;						// Optional
		rasterizer.depthBiasSlopeFactor = 0.0f;					// Optional

		return rasterizer;
	}

	VkPipelineMultisampleStateCreateInfo GetPipelineMultisampleStateCreateInfo()
	{
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;					// Optional
		multisampling.pSampleMask = nullptr;				    // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE;			// Optional
		multisampling.alphaToOneEnable = VK_FALSE;				// Optional

		return multisampling;
	}

	VkPipelineDepthStencilStateCreateInfo GetPipelineDepthStencilStateCreateInfo()
	{
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;           // for new depth values to be written to the depth buffer
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;  // lower depth is closer
		depthStencil.depthBoundsTestEnable = VK_FALSE;     // Optional
		depthStencil.minDepthBounds = 0.0f;               // Optional
		depthStencil.maxDepthBounds = 1.0f;               // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};                           // Optional
		depthStencil.back = {};                            // Optional

		return depthStencil;
	}

	VkPipelineColorBlendAttachmentState GetPipelineColorBlendAttachmentState()
	{
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // RGBA channel
		colorBlendAttachment.blendEnable = VK_FALSE;						                                                                             // False -  framebuffer unmodified
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		                                                                             // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	                                                                             // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;				                                                                             // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		                                                                             // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	                                                                             // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;				                                                                             // Optional

		return colorBlendAttachment;
	}

	VkPipelineColorBlendStateCreateInfo GetPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
	{
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;		// set true to combine color values using bitwise operation
		colorBlending.logicOp = VK_LOGIC_OP_COPY;	// Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;		// Optional
		colorBlending.blendConstants[1] = 0.0f;		// Optional
		colorBlending.blendConstants[2] = 0.0f;		// Optional
		colorBlending.blendConstants[3] = 0.0f;		// Optional

		return colorBlending;
	}

	VkPipelineLayoutCreateInfo GetPipelineLayoutCreateInfo(VkDescriptorSetLayout* descriptorSetLayoutPtr, VkPushConstantRange* pushConstantRangesPtr)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;				                  // a descriptor set layout for uniform buffer object is set
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayoutPtr;              // specify descriptor set layout
		pipelineLayoutInfo.pushConstantRangeCount = 1;		                  // a push constant range is set
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRangesPtr;	      // specify push constant range

		return pipelineLayoutInfo;
	}

	VkGraphicsPipelineCreateInfo GetGraphicsPipelineCreateInfo(
		VkPipelineLayout layout,
		VkPipelineShaderStageCreateInfo* shaderStages,
		VkPipelineVertexInputStateCreateInfo* vertexInputInfo,
		VkPipelineInputAssemblyStateCreateInfo* inputAssembly,
		VkPipelineViewportStateCreateInfo* viewportState,
		VkPipelineRasterizationStateCreateInfo* rasterizer,
		VkPipelineMultisampleStateCreateInfo* multisampling,
		VkPipelineDepthStencilStateCreateInfo* depthStencil,
		VkPipelineColorBlendStateCreateInfo* colorBlending,
		VkPipelineDynamicStateCreateInfo* dynamicState,
		const VkPipelineLayout& pipelineLayout,
		const VkRenderPass& renderPass
	)
	{
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;						// vertex and fragment shader
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = vertexInputInfo;
		pipelineInfo.pInputAssemblyState = inputAssembly;
		pipelineInfo.pViewportState = viewportState;
		pipelineInfo.pRasterizationState = rasterizer;
		pipelineInfo.pMultisampleState = multisampling;
		pipelineInfo.pDepthStencilState = depthStencil;
		pipelineInfo.pColorBlendState = colorBlending;
		pipelineInfo.pDynamicState = dynamicState;
		pipelineInfo.layout = pipelineLayout;			    // pipeline layout
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// Optional - usage for piepline derivatives
		pipelineInfo.basePipelineIndex = -1;				// Optional

		return pipelineInfo;
	}

	VkImageCreateInfo GetImageCreateInfo(
		uint32 width,
		uint32 height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkImageLayout layout
	)
	{
		// specify image creation info
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = layout;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;         // multisampling-related
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // If there are more than two queues using the image, then you should use VK_SHARING_MODE_CONCURRENT

		return imageInfo;
	}

	VkImageViewCreateInfo GetImageViewCreateInfo(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		uint32 mipLevels
	)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;			// 2D image in most cases.
		imageViewCreateInfo.format = format;						    // follw the format of the given swapchain image
		imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;		// first array layer accessible to the view
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;			// first mipmap level accessible to the view
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = mipLevels;	// number of mipmap levels accessible to the view
		imageViewCreateInfo.pNext = nullptr;

		return imageViewCreateInfo;
	}

	VkBufferCreateInfo GetBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;               // size of the buffer in bytes
		bufferInfo.usage = usage;             // indicate the purpose of the data in the buffer
		bufferInfo.sharingMode = sharingMode; // buffer will only be used by the graphics queue family

		return bufferInfo;
	}
}