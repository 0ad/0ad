#ifndef __SIMPLECONSTRAINTS_H__
#define __SIMPLECONSTRAINTS_H__

#include "constraint.h"
#include "map.h"

class NullConstraint : public Constraint {
public:
    virtual bool allows(Map* m, int x, int y);
};

#endif
