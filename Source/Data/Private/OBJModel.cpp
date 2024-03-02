#include "OBJModel.h"

OBJModel::OBJModel(const std::string modelPath, const std::string texturePath) 
	: 
	_modelPath(modelPath), 
	_texturePath(texturePath)
{
	// create texture image for model.
	GDescriptorManager->CreateTextureImage(_texturePath, _vkTextureImage, _vkTextureImageMemory);
	LoadModel();
}


OBJModel::~OBJModel()
{
	GDescriptorManager->DestroyTextureImage(_vkTextureImage);
	GDescriptorManager->DestroyTextureImageMemory(_vkTextureImageMemory);
}

void OBJModel::LoadModel() 
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	// load obj model
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _modelPath.c_str()))
		MK_THROW(warn + err);
	
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			// assume every vertex is unique
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // flip y coordinate because obj format assumes (0,0) is bottom left while vulkan assumes (0,0) is top left
			};
			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0)
			{
				// if key does not exist, add it to the map and the vertices vector
				uniqueVertices[vertex] = SafeStaticCast<size_t, uint32>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}