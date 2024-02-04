#include <iostream>
#include "MKRenderer.h"


MKRenderer::MKRenderer() 
	: _window(false),
	  _instance()
{}

MKRenderer::~MKRenderer()
{
	std::cout << "MKRenderer Destroyed" << std::endl;
}

void MKRenderer::Render()
{
	while (!_window.ShouldClose()) 
	{
		_window.PollEvents();
	}
}
