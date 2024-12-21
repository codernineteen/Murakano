#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "Utilities.h"

class Camera
{
public:
	Camera(VkExtent2D extent);
	~Camera();

	void UpdateCameraPositionX(float moveSpeed);
	void UpdateCameraPositionZ(float moveSpeed);
	void UpdateCameraRotationHorizontal(float rotationSpeed);
	void UpdateCameraRotationVertical(float rotationSpeed);

	void UpdateViewTarget();


	/* event handlers */
	void OnMouseMove(GLFWwindow* windowPtr, float deltaTime);
	void OnMouseDown(GLFWwindow* windowPtr, float deltaTime);
	void OnMouseUp() { _mouseActivated = false; };


#ifdef USE_HLSL
	XMMATRIX GetViewMatrix()        const { return _viewMat; }
	XMMATRIX GetProjectionMatrix()  const { return _projectionMat; }
#else
	glm::mat4 GetViewMatrix()       const { return _viewMat; }
	glm::mat4 GetProjectionMatrix() const { return _projectionMat; }
#endif

private:
	/* murakan instance */
	VkExtent2D _extent;

	/* view, projections settings */
	double _mouseX = 0.0f;
	double _mouseY = 0.0f;
	bool _mouseActivated = false;
#ifdef USE_HLSL
	XMVECTOR _camUp = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	XMVECTOR _camForward = XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR _camRight = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	XMVECTOR _camPos = XMVectorSet(4.0f, 0.0f, 0.0f, 0.0f);

	XMVECTOR _focus = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR _yaw = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	XMVECTOR _roll = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR _pitch = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	XMMATRIX _viewInverseMat = XMMatrixIdentity();
	XMMATRIX _viewMat = XMMatrixIdentity();
	XMMATRIX _projectionMat = XMMatrixIdentity();
#else
	glm::vec3 _upDirection      = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 _forwardDirection = glm::vec3(-1.0f, 0.0f, 0.0f);
	glm::vec3 _rightDirection   = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 _cameraPosition   = glm::vec3(4.0f, 0.0f, 0.0f);
	glm::vec3 _focus    = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 _viewMat          = glm::mat4(1.0f);
	glm::mat4 _projectionMat    = glm::mat4(1.0f);
#endif
};