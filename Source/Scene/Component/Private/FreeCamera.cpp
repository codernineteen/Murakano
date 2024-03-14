#include "FreeCamera.h"

FreeCamera::FreeCamera(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef)
	: _mkDeviceRef(mkDeviceRef), _mkSwapchainRef(mkSwapchainRef)
{
	// Initialize camera matrices
	
	// Transform into view space
	auto viewMat = XMMatrixLookAtLH(_cameraPosition, _focusPosition, _upDirection);
	_viewMat = XMMatrixTranspose(viewMat);

	// update focus direction and right direction
	_forwardDirection = XMVector3Normalize(_focusPosition - _cameraPosition);
	_rightDirection = XMVector3Normalize(XMVector3Cross(_upDirection, _forwardDirection));

	/**
	* Perspective projection
	* 1. fovy : 45 degree field of view
	* 2. aspect ratio : swapchain extent width / swapchain extent height
	* 3. near plane : 0.1f
	* 4. far plane : 10.0f
	*/
	auto extent = _mkSwapchainRef.GetSwapchainExtent();
	auto projectionMat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f),
		extent.width / SafeStaticCast<uint32, float>(extent.height),
		0.1f,
		10.0f
	);
	_projectionMat = XMMatrixTranspose(projectionMat);
}

FreeCamera::~FreeCamera()
{

}

void FreeCamera::UpdateCameraPositionX(float moveSpeed)
{
	_cameraPosition += moveSpeed * _rightDirection;
	_focusPosition += moveSpeed * _rightDirection;
}

void FreeCamera::UpdateCameraPositionZ(float moveSpeed)
{
	_cameraPosition += moveSpeed * _forwardDirection;
	_focusPosition += moveSpeed * _forwardDirection;
}

void FreeCamera::UpdateCameraRotationHorizontal(float rotationSpeed)
{
	// Create a rotation matrix around the up axis
	auto rotationMatrix = XMMatrixRotationAxis(_upDirection, XMConvertToRadians(rotationSpeed));

	// Rotate the forward direction
	_forwardDirection = XMVector3TransformNormal(_forwardDirection, rotationMatrix);

	// Recompute the right and up directions
	_rightDirection = XMVector3Normalize(XMVector3Cross(_upDirection, _forwardDirection));
	_upDirection = XMVector3Normalize(XMVector3Cross(_forwardDirection, _rightDirection));

	// Update the focus position
	_focusPosition = _cameraPosition + _forwardDirection;
}

void FreeCamera::UpdateCameraRotationVertical(float rotationSpeed)
{
	// Create a rotation matrix around the right axis
	auto rotationMatrix = XMMatrixRotationAxis(_rightDirection, XMConvertToRadians(rotationSpeed));

	// Rotate the forward direction
	_forwardDirection = XMVector3TransformNormal(_forwardDirection, rotationMatrix);

	// Recompute the right and up directions
	_rightDirection = XMVector3Normalize(XMVector3Cross(_upDirection, _forwardDirection));
	_upDirection = XMVector3Normalize(XMVector3Cross(_forwardDirection, _rightDirection));

	// Update the focus position
	_focusPosition = _cameraPosition + _forwardDirection;
}


void FreeCamera::UpdateViewTarget()
{
    // Update view matrix
	auto viewMat = XMMatrixLookAtLH(_cameraPosition, _focusPosition, _upDirection);
	_viewMat = XMMatrixTranspose(viewMat);
}