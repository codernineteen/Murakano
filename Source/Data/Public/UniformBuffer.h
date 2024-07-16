#pragma once

#include "Types.h"

struct UniformBufferObject
{
#ifdef USE_HLSL
	/* (model x view x projection) transformation matrix in HLSL */
	alignas (16)XMMATRIX mvpMat = XMMatrixIdentity(); // initialize to identity matrix
	alignas (16)XMMATRIX viewInverseMat = XMMatrixIdentity(); // initialize to identity matrix
#else
	/* transformation matrix in GLSL */
	alignas (16)glm::mat4 modelMat;
	alignas (16)glm::mat4 viewMat;
	alignas (16)glm::mat4 projMat;
#endif
};