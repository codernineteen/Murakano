#include "MKWindow.h"

MKWindow::MKWindow(bool isResizable)
{
    glfwInit(); // initialize glfw
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, isResizable ? GLFW_TRUE : GLFW_FALSE); // disable resize screen
    _window = glfwCreateWindow(WIDTH, HEIGHT, "Murakano", nullptr, nullptr); // create window
    glfwSetWindowUserPointer(_window, this); // store an arbitrary pointer in the callback.
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

MKWindow::~MKWindow()
{
    glfwDestroyWindow(_window);
    glfwTerminate();

#ifndef NDEBUG
    MK_LOG("glfw window destroyed");
#endif
}

void MKWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto windowClass = reinterpret_cast<MKWindow*>(glfwGetWindowUserPointer(window));
    windowClass->framebufferResized = true;
}
