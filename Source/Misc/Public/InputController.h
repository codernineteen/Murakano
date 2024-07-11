#pragma once

#include <GLFW/glfw3.h>

#include "FreeCamera.h"
#include "Utilities.h"

class InputController
{
public:
	InputController(GLFWwindow* windowPtr, FreeCamera& camera);
	~InputController();
	void MoveInPlaneXY(float deltaTime);
	void RotateCamera(float deltaTime);
	void Update(float deltaTime);

public:
	struct KeyboardMappings 
	{
		int moveLeft = GLFW_KEY_A;
		int moveRight = GLFW_KEY_D;
		int moveBackward = GLFW_KEY_S;
		int moveForward = GLFW_KEY_W;
		int rotateUp = GLFW_KEY_UP;
		int rotateDown = GLFW_KEY_DOWN;
		int rotateLeft = GLFW_KEY_LEFT;
		int rotateRight = GLFW_KEY_RIGHT;
	};


private:
	GLFWwindow* _windowPtr;
	FreeCamera& _camera;
	KeyboardMappings _keyMaps{};
	float _moveSpeed = 3.0f;
	float _rotationSpeed = 30.0f;
};