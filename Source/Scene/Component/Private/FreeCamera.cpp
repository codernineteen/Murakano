#include "FreeCamera.h"

FreeCamera::FreeCamera(MKDevice& mkDeviceRef, MKSwapchain& mkSwapchainRef)
	: _mkDeviceRef(mkDeviceRef), _mkSwapchainRef(mkSwapchainRef)
{
	// Initialize camera matrices
	
	// Transform into view space
	auto viewMat = dx::XMMatrixLookAtLH(dx::XMVECTOR{ 4.0f, 0.0f, 4.0f }, dx::XMVECTOR{ 0.0f, 0.0f, 0.0f }, dx::XMVECTOR{ 0.0f, 0.0f, -1.0f });
	_viewMat = dx::XMMatrixTranspose(viewMat);
	/**
	* Perspective projection
	* 1. fovy : 45 degree field of view
	* 2. aspect ratio : swapchain extent width / swapchain extent height
	* 3. near plane : 0.1f
	* 4. far plane : 10.0f
	*/
	auto extent = _mkSwapchainRef.GetSwapchainExtent();
	auto projectionMat = dx::XMMatrixPerspectiveFovLH(
		dx::XMConvertToRadians(45.0f),
		extent.width / SafeStaticCast<uint32, float>(extent.height),
		0.1f,
		10.0f
	);
	_projectionMat = dx::XMMatrixTranspose(projectionMat);
}

FreeCamera::~FreeCamera()
{

}

void FreeCamera::UpdateViewTarget(dx::XMVECTOR focusPosition)
{
    // Update view matrix
	auto viewMat = dx::XMMatrixLookAtLH(dx::XMVECTOR{ 4.0f, 0.0f, 4.0f }, focusPosition, dx::XMVECTOR{ 0.0f, 0.0f, -1.0f });
	_viewMat = dx::XMMatrixTranspose(viewMat);
}
