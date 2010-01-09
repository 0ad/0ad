/* Copyright (C) 2010 Wildfire Games.
 * ...the usual copyright header...
 */

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpExample.h"

// ... any other includes needed ...

class CCmpExample : public ICmpExample
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		// ...
	}

	DEFAULT_COMPONENT_ALLOCATOR(Example)

	// ... member variables ...

	virtual void Init(const CSimContext& context, const CParamNode& paramNode)
	{
		// ...
	}

	virtual void Deinit(const CSimContext& context)
	{
		// ...
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// ...
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		// ...
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg)
	{
		// ...
	}

	// ... Implementation of interface functions: ...
	virtual int DoWhatever(int x, int y)
	{
		return x+y;
	}
};

REGISTER_COMPONENT_TYPE(Example)
