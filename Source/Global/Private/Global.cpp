#include "Global.h"
#include "CommandService.h"
#include "DescriptorManager.h"
#include "Allocator.h"

MKCommandService* GCommandService = nullptr;
MKDescriptorManager* GDescriptorManager = nullptr;
Allocator* GAllocator = nullptr;

class MKGlobal
{
public:
	MKGlobal()
	{
		GCommandService    = new MKCommandService(); // command service will be deleted in MKDevice destructor
		GDescriptorManager = new MKDescriptorManager(); // descriptor manager will be deleted in MKDevice destructor
		GAllocator         = new Allocator();
	}

	~MKGlobal()
	{
	}
} GlobalInstance;