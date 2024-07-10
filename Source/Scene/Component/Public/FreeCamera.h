#pragma once

#include "Device.h"
#include "Swapchain.h"
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
#ifdef USE_HLSL
	XMMATRIX GetViewMatrix()        const { return _viewMat; }
	XMMATRIX GetProjectionMatrix()  const { return _projectionMat; }
#else
	glm::mat4 GetViewMatrix()       const { return _viewMat; }
	glm::mat4 GetProjectionMatrix() const { return _projectionMat; }
#endif

private:
	/* murakan instance */
	MKDevice&           _mkDeviceRef;
	MKSwapchain&        _mkSwapchainRef;

	/* view, projections settings */
#ifdef USE_HLSL
	XMVECTOR _upDirection = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	XMVECTOR _forwardDirection = XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR _rightDirection = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	XMVECTOR _cameraPosition = XMVectorSet(4.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR _focusPosition = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMMATRIX _viewMat = XMMatrixIdentity();
	XMMATRIX _projectionMat = XMMatrixIdentity();
#else
	glm::vec3 _upDirection      = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 _forwardDirection = glm::vec3(-1.0f, 0.0f, 0.0f);
	glm::vec3 _rightDirection   = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 _cameraPosition   = glm::vec3(4.0f, 0.0f, 0.0f);
	glm::vec3 _focusPosition    = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 _viewMat          = glm::mat4(1.0f);
	glm::mat4 _projectionMat    = glm::mat4(1.0f);
#endif
};