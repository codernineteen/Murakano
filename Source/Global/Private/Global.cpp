#include "Global.h"
#include "MKCommandService.h"
#include "MKDescriptorManager.h"

MKCommandService* GCommandService = nullptr;
MKDescriptorManager* GDescriptorManager = nullptr;

class MKGlobal
{
public:
	MKGlobal()
	{
		GCommandService = new MKCommandService(); // command service will be deleted in MKDevice destructor
		GDescriptorManager = new MKDescriptorManager(); // descriptor manager will be deleted in MKDevice destructor
	}

	~MKGlobal()
	{
		
	}
} GlobalInstance;