#ifndef __CONSTRAINT_H__
#define __CONSTRAINT_H__

class Constraint
{
public:
	Constraint(void);
	virtual ~Constraint(void);
	virtual bool allows(class Map* m, int x, int y) = 0;
};

#endif
