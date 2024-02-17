#include "MKRenderer.h"

MKRenderer::MKRenderer() 
	: 
	_mkWindow(false), 
	_mkInstance(), 
	_mkDevice(_mkWindow, _mkInstance), 
	_mkSwapchain(_mkDevice),
	_mkGraphicsPipeline(_mkDevice, _mkSwapchain)
{
}

MKRenderer::~MKRenderer() 
{
}

void MKRenderer::Render()
{
	while (!_mkWindow.ShouldClose()) 
	{
		_mkWindow.PollEvents();
		_mkGraphicsPipeline.DrawFrame();
	}
}
