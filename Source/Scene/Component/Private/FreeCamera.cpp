#include "FreeCamera.h"

FreeCamera::FreeCamera(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef)
	: _mkDeviceRef(mkDeviceRef), _mkSwapchainRef(mkSwapchainRef)
{
	auto extent = _mkSwapchainRef.GetSwapchainExtent();
	// Transform into view space
#ifdef USE_HLSL
	auto viewMat    = XMMatrixLookAtLH(_cameraPosition, _focusPosition, _upDirection);
	_viewMat        = XMMatrixTranspose(viewMat);
	_viewInverseMat = XMMatrixInverse(nullptr, viewMat);

	// update focus direction and right direction
	_forwardDirection = XMVector3Normalize(_focusPosition - _cameraPosition);
	_rightDirection   = XMVector3Normalize(XMVector3Cross(_forwardDirection, _upDirection));

	/**
	* Perspective projection
	* 1. fovy : 45 degree field of view
	* 2. aspect ratio : swapchain extent width / swapchain extent height
	* 3. near plane : 0.1f
	* 4. far plane : 10.0f
	*/
	auto projectionMat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f),
		extent.width / SafeStaticCast<uint32, float>(extent.height),
		0.1f,
		10.0f
	);
	_projectionMat = XMMatrixTranspose(projectionMat);
#else
	_viewMat = glm::lookAt(_cameraPosition, _focusPosition, _upDirection);

	// update focus direction and right direction
	_forwardDirection = glm::normalize(_focusPosition - _cameraPosition);
	_rightDirection = glm::normalize(glm::cross(_upDirection, _forwardDirection));

	_projectionMat = glm::perspective(
		glm::radians(45.0f),
		extent.width / SafeStaticCast<uint32, float>(extent.height),
		0.1f,
		10.0f
	);
	_projectionMat[1][1] *= -1; // flip y coordinate
#endif
}

FreeCamera::~FreeCamera()
{

}

void FreeCamera::UpdateCameraPositionX(float moveSpeed)
{
	_cameraPosition += moveSpeed * _rightDirection;
	_focusPosition  += moveSpeed * _rightDirection;
}

void FreeCamera::UpdateCameraPositionZ(float moveSpeed)
{
	_cameraPosition += moveSpeed * _forwardDirection;
	_focusPosition  += moveSpeed * _forwardDirection;
}

void FreeCamera::UpdateCameraRotationHorizontal(float rotationSpeed)
{
	/**
	* How to rotate camera around given axis
	* 1. Create a rotation matrix around the axis
	* 2. Rotate the forward direction
	* 3. Recompute the right and up directions
	* 4. Update the focus position
	*/
#ifdef USE_HLSL
	auto rotationMatrix = XMMatrixRotationAxis(_upDirection, XMConvertToRadians(rotationSpeed));
	rotationMatrix      = XMMatrixTranspose(rotationMatrix);

	_forwardDirection = XMVector3TransformNormal(_forwardDirection, rotationMatrix);
	_rightDirection   = XMVector3Normalize(XMVector3Cross(_forwardDirection, _upDirection));
	_upDirection      = XMVector3Normalize(XMVector3Cross(_rightDirection, _forwardDirection));

	_focusPosition = _cameraPosition + _forwardDirection;
#else
	auto rotationMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed), _upDirection);

	_forwardDirection = glm::normalize(glm::vec3(rotationMat * glm::vec4(_forwardDirection, 0.0f)));
	_rightDirection   = glm::normalize(glm::cross(_upDirection, _forwardDirection));
	_upDirection      = glm::normalize(glm::cross(_forwardDirection, _rightDirection));

	_focusPosition = _cameraPosition + _forwardDirection;
#endif
}

void FreeCamera::UpdateCameraRotationVertical(float rotationSpeed)
{
#ifdef USE_HLSL
	auto rotationMatrix = XMMatrixRotationAxis(_rightDirection, XMConvertToRadians(rotationSpeed));
	rotationMatrix      = XMMatrixTranspose(rotationMatrix);

	_forwardDirection = XMVector3TransformNormal(_forwardDirection, rotationMatrix);
	_rightDirection   = XMVector3Normalize(XMVector3Cross(_upDirection, _forwardDirection));
	_upDirection      = XMVector3Normalize(XMVector3Cross(_forwardDirection, _rightDirection));

	_focusPosition = _cameraPosition + _forwardDirection;
#else
	auto rotationMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed), _rightDirection);
	
	_forwardDirection = glm::normalize(glm::vec3(rotationMat * glm::vec4(_forwardDirection, 0.0f)));
	_rightDirection   = glm::normalize(glm::cross(_upDirection, _forwardDirection));
	_upDirection      = glm::normalize(glm::cross(_forwardDirection, _rightDirection));

	_focusPosition = _cameraPosition + _forwardDirection;
#endif
}


void FreeCamera::UpdateViewTarget()
{
#ifdef USE_HLSL
    // Update view matrix
	auto viewMat    = XMMatrixLookAtLH(_cameraPosition, _focusPosition, _upDirection);
	_viewMat        = XMMatrixTranspose(viewMat);
	_viewInverseMat = XMMatrixInverse(nullptr, viewMat);
#else
	_viewMat = glm::lookAt(_cameraPosition, _focusPosition, _upDirection);
#endif
}