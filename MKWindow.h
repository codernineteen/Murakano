#pragma once

#define GLFW3_INCLUDE_VULKAN

// external
#include <GLFW/glfw3.h>

// internal
#include "Utilities.h"

// global
const int WIDTH = 1920;
const int HEIGHT = 1080;

// [MKWindow class]
// - Responsibility :
//     - abstraction of window system using GLFW
// - Dependency :
//     GLFW
class MKWindow
{
public:
	MKWindow() = default;
	MKWindow(bool isResizable);
	~MKWindow();
	inline void PollEvents() {	glfwPollEvents(); }
	inline bool ShouldClose() { return glfwWindowShouldClose(window); }

private:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
	GLFWwindow* window;
	bool framebufferResized = false;
};

