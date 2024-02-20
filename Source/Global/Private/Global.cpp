#include "Global.h"
#include "MKCommandService.h"

MKCommandService* GCommandService = nullptr;

class MKGlobal
{
public:
	MKGlobal()
	{
		GCommandService = new MKCommandService(); // command service will be deleted in MKDevice destructor
	}

	~MKGlobal()
	{
		
	}
} GlobalInstance;