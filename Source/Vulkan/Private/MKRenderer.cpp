#include "MKRenderer.h"

MKRenderer::MKRenderer() 
	: 
	_mkWindow(true), 
	_mkInstance(), 
	_mkDevice(_mkWindow, _mkInstance), 
	_mkSwapchain(_mkDevice),
	_mkGraphicsPipeline(_mkDevice, _mkSwapchain),
	_mkRaytracer(_mkDevice, _mkGraphicsPipeline)
{
	_mkRaytracer.BuildRayTracer(_mkGraphicsPipeline.vikingRoom, _mkGraphicsPipeline.vikingRoomInstance);
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

	_mkDevice.WaitUntilDeviceIdle();
}
