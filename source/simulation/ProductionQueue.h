/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_PRODUCTIONQUEUE
#define INCLUDED_PRODUCTIONQUEUE

#include "EntityHandles.h"
#include "scripting/ScriptableObject.h"
#include <vector>

class CEntity;

class CProductionItem : public CJSObject<CProductionItem>
{
public:
	int m_type;
	CStrW m_name;
	float m_totalTime;		// how long this production takes
	float m_elapsedTime;	// how long we've been working on this production

	CProductionItem( int type, const CStrW& name, float totalTime );
	~CProductionItem();

	void Update( int timestep );

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
	void Update( int timestep );
	void CancelAll();

	jsval JSI_GetLength( JSContext* cx );
	jsval_t JSI_Get( JSContext* cx, uintN argc, jsval* argv );
	bool JSI_Cancel( JSContext* cx, uintN argc, jsval* argv );

	static void ScriptingInit();
};

#endif
