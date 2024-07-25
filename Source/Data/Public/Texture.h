#pragma once

#include <stb_image.h>

#include "VulkanType.h"
#include "Utilities.h"
#include "Device.h"
#include "CommandService.h"
#include "Allocator.h"

// an abstract class to combine texture resources altogether
struct Texture
{
	Texture();
	~Texture();

	/* texture api */
	void BuildTextureFromExternal(MKDevice& device, const std::string& name, const std::string& path);
	void BuildGenericTexture();
	void CreateTextureImage(const std::string& name);
	void CreateTextureImageView(MKDevice& device);
	void DestroyTexture(MKDevice& device);
	
	/* member field */
	std::string texturePath;
	
	VkImageAllocated  image;
	VkImageView       imageView;
	VkSampler         imageSampler{ VK_NULL_HANDLE }; // for bindless texture sampling, it is optional to initialize sampler
};