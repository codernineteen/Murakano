#pragma once

#include "Types.h"

struct UniformBufferObject
{
#ifdef USE_HLSL
	/* transformation matrices */
	alignas (16)XMMATRIX viewMat = XMMatrixIdentity();
	alignas (16)XMMATRIX projMat = XMMatrixIdentity();
	alignas (16)XMMATRIX mvpMat  = XMMatrixIdentity(); 
	alignas (16)XMMATRIX viewInverseMat = XMMatrixIdentity();
	
	/* scene environment with default values */
	alignas (16)XMVECTOR ambientColor  = XMVectorSet(0.1f, 0.1f, 0.1f, 1.0f);
	alignas (16)XMVECTOR sunlightDir   = XMVectorSet(-12.0f, -11.0f, 0.0f, 0.0f);
	alignas (16)XMVECTOR sunlightColor = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
#else
	/* (DEPRECATED) transformation matrix in GLSL */
	alignas (16)glm::mat4 modelMat;
	alignas (16)glm::mat4 viewMat;
	alignas (16)glm::mat4 projMat;
#endif
};