#include "OBJModel.h"

OBJModel::OBJModel(MKDevice& mkDeviceRef) : _mkDeviceRef(mkDeviceRef)
{
// initial model matrix is identitiy matrix
#ifdef USE_HLSL
	modelMatrix = XMMatrixIdentity();
#else
	modelMatrix = glm::mat4(1.0f);
#endif
}


OBJModel::~OBJModel()
{
}

void OBJModel::LoadModel(const std::string& modelPath, const std::vector<TextureMetadata>& textureParams) 
{
	// path field initialize
	_modelPath = modelPath;

	// build texture , order of paths is : { diffuse, specular, normal }
	int it = 0;
	textures.resize(textureParams.size());
	for (const auto& metadata : textureParams)
	{
		std::unique_ptr<Texture> texture = std::make_unique<Texture>();
		texture->BuildTexture(_mkDeviceRef, metadata.first, metadata.second);
		textures[it] = std::move(texture); // move ownership of texture in this for-loop to textures vector
		it++;
	}

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	// load obj model
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _modelPath.c_str()))
		MK_THROW(warn + err); // if LoadObj return false, throw an error

	for (size_t s_it = 0; s_it < shapes.size(); s_it++)
	{
		size_t shape_index_offset = 0;
		for (size_t f_it = 0; f_it < shapes[s_it].mesh.num_face_vertices.size(); f_it++)
		{
			int face_vertices = shapes[s_it].mesh.num_face_vertices[f_it];

			for (size_t v_it = 0; v_it < face_vertices; v_it++)
			{
				// current index
				tinyobj::index_t idx = shapes[s_it].mesh.indices[shape_index_offset + v_it];

				Vertex vertex{};
				// position
				vertex.pos = {
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2]
				};

				// normal vector
				if (!attrib.normals.empty() && idx.normal_index >= 0)
				{
					vertex.normal = {
						attrib.normals[3 * idx.normal_index + 0],
						attrib.normals[3 * idx.normal_index + 1],
						attrib.normals[3 * idx.normal_index + 2],
					};
				}

				// texture coordinates
				if (!attrib.texcoords.empty() && idx.texcoord_index >= 0)
				{
					vertex.texCoord = {
						attrib.texcoords[2 * idx.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * idx.texcoord_index + 1] // flip 'y' component because vulkan assumes (0,0) is top left
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					// if key does not exist, add it to the map and the vertices vector
					uniqueVertices[vertex] = static_cast<uint32>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
			
			shape_index_offset += face_vertices;
		}

		// if normal attribute is not given, compute normal for each vertices
		if (attrib.normals.empty())
		{
			for (size_t it = 0; it < indices.size(); it += 3)
			{
				Vertex& v0 = vertices[indices[it + 0]];
				Vertex& v1 = vertices[indices[it + 1]];
				Vertex& v2 = vertices[indices[it + 2]];

#ifdef USE_HLSL
				XMVECTOR p0 = XMLoadFloat3(&v0.pos);
				XMVECTOR p1 = XMLoadFloat3(&v1.pos);
				XMVECTOR p2 = XMLoadFloat3(&v2.pos);

				// compute normal vector by tacking cross product of two edges
				XMVECTOR n = XMVector3Normalize(XMVector3Cross(p0 - p1, p0 - p2));
				// store result
				XMStoreFloat3(&v0.normal, n);
				XMStoreFloat3(&v1.normal, n);
				XMStoreFloat3(&v2.normal, n);
#else
				glm::vec3 n = glm::normalize(glm::cross((v1.pos - v0.pos), (v2.pos - v0.pos)));
				v0.nrm = n;
				v1.nrm = n;
				v2.nrm = n;
#endif
			}
		}
		
	}
}

void OBJModel::DestroyModel()
{
	for (auto& texture : textures)
	{
		texture->DestroyTexture(_mkDeviceRef); // destroy texture resources before destructing model
	}
}

void OBJModel::Scale(float scaleX, float scaleY, float scaleZ)
{
#ifdef USE_HLSL
	XMMATRIX scaleMat = XMMatrixScaling(scaleX, scaleY, scaleZ);
	modelMatrix = XMMatrixMultiply(scaleMat, modelMatrix);
#else
	modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleX, scaleY, scaleZ));
#endif
}
/**
* Rotation
* - HLSL : XMMatrixRotationX, XMMatrixRotationY, XMMatrixRotationZ
* - GLSL : glm::rotate
*	[parameters]
*	 - matrix : transformation target
*	 - deg : degree of rotation in radians
*	 - axis : axis of rotation
*/
void OBJModel::RotateX(float deg)
{
#ifdef USE_HLSL
	XMMATRIX rotMat = XMMatrixRotationX(XMConvertToRadians(deg));
	modelMatrix = XMMatrixMultiply(rotMat, modelMatrix);
#else
	modelMatrix = glm::rotate(modelMatrix, glm::radians(deg), glm::vec3(1.0f, 0.0f, 0.0f));
#endif
}

void OBJModel::RotateY(float deg)
{
#ifdef USE_HLSL
	XMMATRIX rotMat = XMMatrixRotationY(XMConvertToRadians(deg));
	modelMatrix = XMMatrixMultiply(rotMat, modelMatrix);
#else
	modelMatrix = glm::rotate(modelMatrix, glm::radians(deg), glm::vec3(0.0f, 1.0f, 0.0f));
#endif
}

void OBJModel::RotateZ(float deg)
{
#ifdef USE_HLSL
	XMMATRIX rotMat = XMMatrixRotationZ(XMConvertToRadians(deg));
	modelMatrix = XMMatrixMultiply(rotMat, modelMatrix);
#else
	modelMatrix = glm::rotate(modelMatrix, glm::radians(deg), glm::vec3(0.0f, 0.0f, 1.0f));
#endif
}