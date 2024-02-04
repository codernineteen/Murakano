#include "MKRenderer.h"

MKRenderer::MKRenderer()
{
	window = MKWindow(false);
}

MKRenderer::~MKRenderer()
{
}

void MKRenderer::Render()
{
	while (!window.ShouldClose()) {
		window.PollEvents();
	}
}
