#pragma once

#include <DirectXMath.h>

namespace dx = DirectX;

struct UniformBufferObject
{
	/* (model x view x projection) transformation matrix */
	dx::XMMATRIX mvpMat = dx::XMMatrixIdentity(); // initialize to identity matrix
};