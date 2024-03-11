#pragma once

#include <tiny_obj_loader.h>

#include "Utilities.h"
#include "MKDescriptorManager.h"
#include "Vertex.h"
#include "Texture.h"

struct OBJModel
{
public:
	OBJModel(MKDevice& mkDeviceRef, const std::string modelPath, const std::string texturePath);
	~OBJModel();
	void LoadModel();

private:
	VkImageAllocated  _vkTextureImage;
	VkImageView       _vkTextureImageView;

private:
	const std::string _modelPath;
	const std::string _texturePath;

public:
	Texture             vikingTexture;
	std::vector<Vertex> vertices;
	std::unordered_map<Vertex, uint32, VertexHash> uniqueVertices; // unique vertices with key as vertex and value as index
	std::vector<uint32> indices;
};