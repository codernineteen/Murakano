#include "MKRenderer.h"


MKRenderer::MKRenderer() 
	: _window(false), _instance(), _device(_window, _instance), _swapchain(_device)
{}

MKRenderer::~MKRenderer(){}

void MKRenderer::Render()
{
	while (!_window.ShouldClose()) 
	{
		_window.PollEvents();
	}
}
