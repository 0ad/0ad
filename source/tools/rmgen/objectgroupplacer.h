#ifndef __OBJECTGROUPPLACER_H__
#define __OBJECTGROUPPLACER_H__

#include "constraint.h"
#include "object.h"

class ObjectGroupPlacer
{
public:
	virtual bool place(class Map* m, Constraint* constr, std::vector<Object*>& ret) = 0;

	ObjectGroupPlacer(void);
	virtual ~ObjectGroupPlacer(void);
};

#endif