#include "MKWindow.h"

MKWindow::MKWindow(bool isResizable)
{
    glfwInit(); // initialize glfw
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, isResizable ? GLFW_TRUE : GLFW_FALSE); // disable resize screen
    window = glfwCreateWindow(WIDTH, HEIGHT, "Murakano", nullptr, nullptr); // create window
    glfwSetWindowUserPointer(window, this); // store an arbitrary pointer in the callback.
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

MKWindow::~MKWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void MKWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto windowClass = reinterpret_cast<MKWindow*>(glfwGetWindowUserPointer(window));
    windowClass->framebufferResized = true;
}
