#include "MKRenderer.h"


MKRenderer::MKRenderer() 
	: _window(false), _instance() 
{}

MKRenderer::~MKRenderer(){}

void MKRenderer::Render()
{
	while (!_window.ShouldClose()) 
	{
		_window.PollEvents();
	}
}
