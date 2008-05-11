#ifndef SIMSTATE_INCLUDED
#define SIMSTATE_INCLUDED

#include <set>
#include <vector>

#include "ps/CStr.h"
#include "maths/Vector3D.h"

class CUnit;
class CEntity;

class SimState
{
public:
	class Entity
	{
	public:
		static Entity Freeze(CUnit* unit);
		CEntity* Thaw();
	private:
		CStrW templateName;
		size_t unitID;
		std::set<CStr> selections;
		size_t playerID;
		CVector3D position;
		float angle;
	};

	class Nonentity
	{
	public:
		static Nonentity Freeze(CUnit* unit);
		CUnit* Thaw();
	private:
		CStrW actorName;
		size_t unitID;
		std::set<CStr> selections;
		CVector3D position;
		float angle;
	};
	
	static SimState* Freeze(bool onlyEntities);
	void Thaw();

private:
	bool onlyEntities;
	std::vector<Entity> entities;
	std::vector<Nonentity> nonentities;
};

#endif // SIMSTATE_INCLUDED
