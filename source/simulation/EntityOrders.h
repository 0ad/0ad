// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#ifndef ENTITY_ORDER_INCLUDED
#define ENTITY_ORDER_INCLUDED

#define ORDER_MAX_DATA 1

#include "EntityHandles.h"

struct SOrderData
{
	union
	{
		struct
		{
			float x;
			float y;
		} location;
		u64 data;  // miscellaneous
	};
	HEntity entity;
};

class CEntityOrder
{
public:
	enum
	{
		ORDER_GOTO_NOPATHING,
		ORDER_GOTO,
		ORDER_PATROL
	} m_type;
	SOrderData m_data[ORDER_MAX_DATA];
};

#endif