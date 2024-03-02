#pragma once

#include <tiny_obj_loader.h>

#include "Utilities.h"
#include "MKDescriptorManager.h"
#include "Vertex.h"

struct OBJModel
{
public:
	OBJModel(const std::string modelPath, const std::string texturePath);
	~OBJModel();
	void LoadModel();

private:
	VkImage         _vkTextureImage;
    VkDeviceMemory  _vkTextureImageMemory;
	VkImageView     _vkTextureImageView;
	VkBuffer        _vkVertexBuffer;
	VkDeviceMemory  _vkVertexBufferMemory;

private:
	const std::string _modelPath;
	const std::string _texturePath;

public:
	std::vector<Vertex> vertices;
	std::unordered_map<Vertex, uint32, VertexHash> uniqueVertices; // unique vertices with key as vertex and value as index
	std::vector<uint32> indices;
};