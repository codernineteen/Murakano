#pragma once

#include <tiny_obj_loader.h>

#include "Utilities.h"
#include "DescriptorManager.h"
#include "Vertex.h"
#include "Texture.h"

struct OBJModel
{
public:
	OBJModel(MKDevice& mkDeviceRef);
	~OBJModel();
	
	/* load and destroy model */
	void LoadModel(const std::string& modelPath, const std::vector<TextureMetadata>& textureParams);
	void DestroyModel();

	/* getter */
	auto GetModelMatrix() const { 
#ifdef USE_HLSL
		auto modelMat = XMMatrixTranspose(modelMatrix);
		return modelMat;
#else
		return modelMatrix;
#endif
	}

	/* model transformations */
	void Scale(float scaleX, float scaleY, float scaleZ);
	void RotateX(float deg);
	void RotateY(float deg);
	void RotateZ(float deg);

private:
	std::string _modelPath;

public:
	/* texture */
	std::vector<std::unique_ptr<Texture>> textures;
	
	/* vertex */
	std::vector<Vertex>                             vertices;
	std::unordered_map<Vertex, uint32, VertexHash>  uniqueVertices; // unique vertices with key as vertex and value as index
	std::vector<uint32>                             indices;

	/* stacked model transformation matrices */
#ifdef USE_HLSL
	XMMATRIX modelMatrix;
#else
	glm::mat4 modelMatrix;
#endif

	MKDevice& _mkDeviceRef;
};

struct OBJInstance
{
	glm::mat4 transform;
	uint32    objIndex{ 0 };
};