#pragma once

// internal
#include "Utilities.h"
#include "Window.h"
#include "Instance.h"
#include "ValidationLayer.h"
#include "Device.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"
#include "CommandService.h"
#include "Raytracer.h"

class MKRenderer
{
public:
	MKRenderer();
	~MKRenderer();
	void Render();

private:
	MKWindow			_mkWindow;
	MKInstance			_mkInstance;
	MKDevice			_mkDevice;
	MKSwapchain	        _mkSwapchain;
	MKGraphicsPipeline	_mkGraphicsPipeline;
};

