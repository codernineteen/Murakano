#pragma once

// internal
#include "Utilities.h"
#include "MKWindow.h"
#include "MKInstance.h"
#include "MKValidationLayer.h"
#include "MKDevice.h"
#include "MKSwapchain.h"
#include "MKCommandService.h"
#include "MKRaytracer.h"

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
};

