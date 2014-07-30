/* Copyright (C) 2014 Wildfire Games.
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

	static std::string GetSchema()
	{
		return "<ref name='anything'/>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		// ...
	}

	virtual void Deinit()
	{
		// ...
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// ...
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		// ...
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
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
