#pragma once

#include "Types.h"

struct UniformBufferObject
{
	/* (model x view x projection) transformation matrix */
	alignas (16)XMMATRIX mvpMat = XMMatrixIdentity(); // initialize to identity matrix
};