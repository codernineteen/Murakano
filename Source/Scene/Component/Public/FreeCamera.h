#pragma once

#include "MKDevice.h"
#include "MKSwapchain.h"
#include "Utilities.h"

class FreeCamera
{
public:
	FreeCamera(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef);
	~FreeCamera();
	
	void UpdateCameraPositionX(float moveSpeed);
	void UpdateCameraPositionZ(float moveSpeed);
	void UpdateCameraRotationHorizontal(float rotationSpeed);
	void UpdateCameraRotationVertical(float rotationSpeed);

	void UpdateViewTarget();

	XMMATRIX GetViewMatrix() const { return _viewMat; }
	XMMATRIX GetProjectionMatrix() const { return _projectionMat; }

private:
	/* murakan instance */
	MKDevice&           _mkDeviceRef;
	MKSwapchain&        _mkSwapchainRef;

	/* view, projections settings */
	XMVECTOR _upDirection = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	XMVECTOR _forwardDirection = XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR _rightDirection = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	XMVECTOR _cameraPosition = XMVectorSet(4.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR _focusPosition = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMMATRIX _viewMat = XMMatrixIdentity();
	XMMATRIX _projectionMat = XMMatrixIdentity();
};