#include "Global.h"
#include "CommandService.h"
#include "DescriptorManager.h"
#include "Raytracer.h"

MKCommandService* GCommandService = nullptr;
MKDescriptorManager* GDescriptorManager = nullptr;
MKRaytracer* GRaytracer = nullptr;

class MKGlobal
{
public:
	MKGlobal()
	{
		GCommandService = new MKCommandService(); // command service will be deleted in MKDevice destructor
		GDescriptorManager = new MKDescriptorManager(); // descriptor manager will be deleted in MKDevice destructor
		GRaytracer = new MKRaytracer();
	}

	~MKGlobal()
	{
		
	}
} GlobalInstance;