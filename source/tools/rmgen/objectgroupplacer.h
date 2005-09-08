#ifndef __OBJECTGROUPPLACER_H__
#define __OBJECTGROUPPLACER_H__

#include "constraint.h"
#include "object.h"

class ObjectGroupPlacer
{
public:
	virtual bool place(class Map* m, int player, Constraint* constr) = 0;

	ObjectGroupPlacer(void);
	virtual ~ObjectGroupPlacer(void);
};

#endif