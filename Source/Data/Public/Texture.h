#pragma once

#include <stb_image.h>

#include "VulkanType.h"
#include "Utilities.h"
#include "Device.h"
#include "CommandService.h"

// an abstract class to combine texture resources altogether
struct Texture
{
	Texture(MKDevice& mkDeviceRef, const std::string texturePath);
	~Texture();

	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	
	VkImageAllocated  image;
	VkImageView       imageView;
	VkSampler         sampler;

	MKDevice& mkDeviceRef;
	const std::string texturePath;
};