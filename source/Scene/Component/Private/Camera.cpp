#include <GLFW/glfw3.h>
#include "Camera.h"

Camera::Camera(VkExtent2D extent)
	: _extent(extent)
{
	// Transform into view space
#ifdef USE_HLSL
	auto viewMat    = XMMatrixLookAtLH(_camPos, _focus, _camUp);
	_viewMat        = XMMatrixTranspose(viewMat);
	_viewInverseMat = XMMatrixInverse(nullptr, viewMat);

	// update focus direction and right direction
	_camForward = XMVector3Normalize(_focus - _camPos);
	_camRight   = XMVector3Normalize(XMVector3Cross(_camForward, _camUp));

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
	_viewMat = glm::lookAt(_camPos, _focus, _camUp);

	// update focus direction and right direction
	_camForward = glm::normalize(_focus - _camPos);
	_camRight = glm::normalize(glm::cross(_camUp, _camForward));

	_projectionMat = glm::perspective(
		glm::radians(45.0f),
		extent.width / SafeStaticCast<uint32, float>(extent.height),
		0.1f,
		10.0f
	);
	_projectionMat[1][1] *= -1; // flip y coordinate
#endif
}

Camera::~Camera()
{

}

void Camera::OnMouseDown(GLFWwindow* windowPtr, float deltaTime)
{
	if(!_mouseActivated)
	{
		_mouseActivated = true;
		glfwGetCursorPos(windowPtr, &_mouseX, &_mouseY);
	}
	else
		OnMouseMove(windowPtr, deltaTime);
}

void Camera::OnMouseMove(GLFWwindow* windowPtr, float deltaTime)
{
	double xpos, ypos;
	glfwGetCursorPos(windowPtr, &xpos, &ypos);

	double deltaX = _mouseX - xpos;
	double deltaY = _mouseY - ypos;

	float horizontalRotation = deltaX * deltaTime * 300.0f;
	float verticalRotation = deltaY * deltaTime * 300.0f;

	UpdateCameraRotationHorizontal(horizontalRotation);
	UpdateCameraRotationVertical(verticalRotation);

	_mouseX = xpos;
	_mouseY = ypos;
}

void Camera::UpdateCameraPositionX(float moveSpeed)
{
	_camPos += moveSpeed * _camRight;
	_focus  += moveSpeed * _camRight;
}

void Camera::UpdateCameraPositionZ(float moveSpeed)
{
	_camPos += moveSpeed * _camForward;
	_focus  += moveSpeed * _camForward;
}

void Camera::UpdateCameraRotationHorizontal(float rotationSpeed)
{
	/**
	* How to rotate camera around given axis
	* 1. Create a rotation matrix around the axis
	* 2. Rotate the forward direction
	* 3. Recompute the right and up directions
	* 4. Update the focus position
	*/
#ifdef USE_HLSL
	auto focusToCam = _camPos - _focus;

	auto rotationMatrix = XMMatrixRotationAxis(_yaw, XMConvertToRadians(rotationSpeed));
	rotationMatrix      = XMMatrixTranspose(rotationMatrix);

	_camPos = XMVector3TransformNormal(focusToCam, rotationMatrix);

	// invert direction
	/*_camForward = XMVector3Normalize(_focus - _camPos);
	_camRight   = XMVector3Normalize(XMVector3Cross(_camForward, _camUp));
	_camUp      = XMVector3Normalize(XMVector3Cross(_camRight, _camForward));*/

#else
	auto rotationMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed), _camUp);

	_camForward = glm::normalize(glm::vec3(rotationMat * glm::vec4(_camForward, 0.0f)));
	_camRight   = glm::normalize(glm::cross(_camUp, _camForward));
	_camUp      = glm::normalize(glm::cross(_camForward, _camRight));

	_focus = _camPos + _camForward;
#endif
}

void Camera::UpdateCameraRotationVertical(float rotationSpeed)
{
#ifdef USE_HLSL
	auto focusToCam = _camPos - _focus;

	auto rotationMatrix = XMMatrixRotationAxis(_pitch, XMConvertToRadians(rotationSpeed));
	rotationMatrix = XMMatrixTranspose(rotationMatrix);

	_camPos = XMVector3TransformNormal(focusToCam, rotationMatrix);

	// invert direction
	/*_camForward = XMVector3Normalize(_focus - _camPos);
	_camRight   = XMVector3Normalize(XMVector3Cross(_camUp, _camForward));
	_camUp      = XMVector3Normalize(XMVector3Cross(_camForward, _camRight));*/

	
#else
	auto rotationMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed), _camRight);
	
	_camForward = glm::normalize(glm::vec3(rotationMat * glm::vec4(_camForward, 0.0f)));
	_camRight   = glm::normalize(glm::cross(_camUp, _camForward));
	_camUp      = glm::normalize(glm::cross(_camForward, _camRight));

	_focus = _camPos + _camForward;
#endif
}


void Camera::UpdateViewTarget()
{
#ifdef USE_HLSL
    // Update view matrix
	auto viewMat    = XMMatrixLookAtLH(_camPos, _focus, _camUp);
	_viewMat        = XMMatrixTranspose(viewMat);
	_viewInverseMat = XMMatrixInverse(nullptr, viewMat);
#else
	_viewMat = glm::lookAt(_camPos, _focus, _camUp);
#endif
}