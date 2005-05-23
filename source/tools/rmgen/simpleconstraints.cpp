#include "stdafx.h"
#include "simpleconstraints.h"

bool NullConstraint::allows(Map* m, int x, int y)
{
	return true;
}
