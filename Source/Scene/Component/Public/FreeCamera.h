#pragma once

#include "MKDevice.h"
#include "MKSwapchain.h"
#include "Utilities.h"

class FreeCamera
{
public:
	FreeCamera(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef);
	~FreeCamera();
	void UpdateViewTarget(dx::XMVECTOR focusPosition);

	dx::XMMATRIX GetViewMatrix() const { return _viewMat; }
	dx::XMMATRIX GetProjectionMatrix() const { return _projectionMat; }

private:
	MKDevice& _mkDeviceRef;
	MKSwapchain& _mkSwapchainRef;
	dx::XMMATRIX _viewMat = dx::XMMatrixIdentity();
	dx::XMMATRIX _projectionMat = dx::XMMatrixIdentity();
};