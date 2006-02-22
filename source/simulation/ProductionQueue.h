#include "Entity.h"
#include "EntityHandles.h"
#include <vector>

#ifndef __PRODUCTIONQUEUE_H__
#define __PRODUCTIONQUEUE_H__

class CProductionItem : public CJSObject<CProductionItem>
{
public:
	int m_type;
	CStrW m_name;
	float m_totalTime;		// how long this production takes
	float m_elapsedTime;	// how long we've been working on this production

	CProductionItem( int type, const CStrW& name, float totalTime );
	~CProductionItem();

	void Update( size_t timestep );

	bool IsComplete();
	float GetProgress();

	jsval JSI_GetProgress( JSContext* cx );

	static void ScriptingInit();
};

class CProductionQueue : public CJSObject<CProductionQueue>
{
	CEntity* m_owner;
	std::vector<CProductionItem*> m_items;
public:
	CProductionQueue( CEntity* owner );
	~CProductionQueue();

	void AddItem( int type, const CStrW& name, float totalTime );
	void Update( size_t timestep );

	jsval JSI_GetLength( JSContext* cx );
	jsval JSI_Get( JSContext* cx, uintN argc, jsval* argv );
	bool JSI_Cancel( JSContext* cx, uintN argc, jsval* argv );

	static void ScriptingInit();
};

#endif
