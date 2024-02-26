#include "Macros.h"
#include "MKRenderer.h"


int main()
{
	MKRenderer renderer;
	renderer.Render();

#ifdef _MSC_VER
#    ifdef NDEBUG
#        pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#    else
#        pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#    endif
#endif
}
