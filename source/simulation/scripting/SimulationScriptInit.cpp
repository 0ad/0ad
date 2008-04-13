#include "precompiled.h"

#include "simulation/Scheduler.h"
#include "simulation/EntityTemplate.h"
#include "simulation/Entity.h"
#include "simulation/Projectile.h"
#include "simulation/TriggerManager.h"
#include "simulation/ProductionQueue.h"
#include "simulation/Technology.h"

void SimulationScriptInit()
{
	CJSProgressTimer::ScriptingInit();
	CEntityTemplate::ScriptingInit();
	CEntity::ScriptingInit();
	CProjectile::ScriptingInit();
	CTrigger::ScriptingInit();
	CProductionItem::ScriptingInit();
	CProductionQueue::ScriptingInit();
	CTechnology::ScriptingInit();

	EntityCollection::Init( "EntityCollection" );
}
