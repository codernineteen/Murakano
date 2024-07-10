#pragma once

#define GLFW_INCLUDE_VULKAN

// external
#include <GLFW/glfw3.h>

// internal
#include "Utilities.h"

// global
const int WIDTH = 1680;
const int HEIGHT = 1050;

// [MKWindow class]
// - Responsibility :
//    - abstraction of window system using GLFW
// - Dependency :
//    - GLFW
class MKWindow
{
public:
	MKWindow() = default;
	MKWindow(bool isResizable);
	~MKWindow();
	inline void PollEvents() {	glfwPollEvents(); }
	inline bool ShouldClose() { return glfwWindowShouldClose(_window); }
	GLFWwindow* GetWindow() const { return _window; }

private:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

public:
	bool framebufferResized = false;

private:
	GLFWwindow* _window;
};

