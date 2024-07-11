#include "InputController.h"

InputController::InputController(GLFWwindow* windowPtr, FreeCamera& camera)
:
	_windowPtr(windowPtr),
	_camera(camera)
{

}

InputController::~InputController()
{

}

void InputController::RotateCamera(float deltaTime)
{
	if (glfwGetKey(_windowPtr, _keyMaps.rotateUp) == GLFW_PRESS)
		_camera.UpdateCameraRotationVertical(-_rotationSpeed * deltaTime);
	if (glfwGetKey(_windowPtr, _keyMaps.rotateDown) == GLFW_PRESS)
		_camera.UpdateCameraRotationVertical(_rotationSpeed * deltaTime);
	if (glfwGetKey(_windowPtr, _keyMaps.rotateLeft) == GLFW_PRESS)
		_camera.UpdateCameraRotationHorizontal(_rotationSpeed * deltaTime);
	if (glfwGetKey(_windowPtr, _keyMaps.rotateRight) == GLFW_PRESS)
		_camera.UpdateCameraRotationHorizontal(-_rotationSpeed * deltaTime);
}

void InputController::MoveInPlaneXY(float deltaTime)
{
	//XMVECTOR forward = _mkWindowRef.GetCamera().GetForward();
	if (glfwGetKey(_windowPtr, _keyMaps.moveForward) == GLFW_PRESS) 
		_camera.UpdateCameraPositionZ(_moveSpeed * deltaTime);
	if (glfwGetKey(_windowPtr, _keyMaps.moveBackward) == GLFW_PRESS) 
		_camera.UpdateCameraPositionZ(-_moveSpeed * deltaTime);
	if (glfwGetKey(_windowPtr, _keyMaps.moveLeft) == GLFW_PRESS) 
		_camera.UpdateCameraPositionX(_moveSpeed * deltaTime);
	if (glfwGetKey(_windowPtr, _keyMaps.moveRight) == GLFW_PRESS) 
		_camera.UpdateCameraPositionX(-_moveSpeed * deltaTime);
}

void InputController::Update(float deltaTime)
{
	MoveInPlaneXY(deltaTime);
	RotateCamera(deltaTime);
}