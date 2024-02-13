#pragma once

// internal
#include "Utilities.h"
#include "MKWindow.h"
#include "MKInstance.h"
#include "MKValidationLayer.h"
#include "MKDevice.h"
#include "MKSwapchain.h"

// [MKRenderer class]
// - Responsibility :
//    This class is top-level class of whole system and make abstraction of rendering pipeline for high-level user.
// - Dependency :
//    MKWindow
class MKRenderer
{
public:
	MKRenderer();
	~MKRenderer();
	void Render();

private:
	MKWindow	_window;
	MKInstance	_instance;
	MKDevice	_device;
	MKSwapchain _swapchain;
};

