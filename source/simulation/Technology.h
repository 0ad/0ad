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

// Holds effects of a technology (research item), as well as its status 
// (unavailable, researched, in progress, etc).
//
// There is a separate CTechnology object for each tech for each player,
// because the status can be different for different players.

#ifndef INCLUDED_TECHNOLOGY
#define INCLUDED_TECHNOLOGY

#include <vector>
#include "scripting/ScriptableComplex.h"
#include "simulation/ScriptObject.h"
#include "ps/Game.h"

class XMBElement;
class CXeromyces;
class CEntity;

class CTechnology : public CJSComplex<CTechnology>
{
	NONCOPYABLE(CTechnology);

	friend class CTechnologyCollection;
	
	struct Modifier
	{
		CStr attribute;
		float value;
		bool isPercent;
		Modifier(): value(0), isPercent(false) {}
	};
	static STL_HASH_SET<CStr, CStr_hash_compare> m_scriptsLoaded;

public:
	CTechnology(const CStrW& name, CPlayer* player);
	~CTechnology() {}

	//JS functions
	static void ScriptingInit();
	bool ApplyEffects( JSContext* cx, uintN argc, jsval* argv );
	bool IsValid( JSContext* cx, uintN argc, jsval* argv );
	bool IsResearched( JSContext* cx, uintN argc, jsval* argv );
	bool IsExcluded( JSContext* cx, uintN argc, jsval* argv );
	size_t GetPlayerID( JSContext* cx, uintN argc, jsval* argv );
	
	void Apply( CEntity* entity );

	bool IsTechValid();
	inline bool IsResearched() { return m_researched; }

	void SetExclusion( bool exclude ) { m_excluded=exclude; }

	bool LoadXml( const VfsPath& filename );
	bool LoadElId( XMBElement ID, CXeromyces& XeroFile );
	bool LoadElReq( XMBElement Req, CXeromyces& XeroFile );
	bool LoadElEffect( XMBElement Effect, CXeromyces& XeroFile, const VfsPath& pathname);

private:
	CStrW m_Name;	// name of the tech file

	CStrW m_Generic;
	CStrW m_Specific;

	CStrW m_Icon;
	int m_IconCell;
	CStrW m_Classes;
	CStrW m_History;

	float m_ReqTime;
	std::vector<CStr> m_ReqEntities;
	std::vector<CStr> m_ReqTechs;

	std::vector<CStr> m_Pairs;
	std::vector<CStr> m_Targets;
	std::vector<Modifier> m_Modifiers;
	std::vector<Modifier> m_Sets;
	
	CPlayer* m_player;	//Which player this tech object belongs to

	CScriptObject m_effectFunction;

	bool m_excluded;
	bool m_researched;
	bool m_inProgress;

	bool HasReqEntities();
	bool HasReqTechs();

	// Hack: shouldn't be part of CJSComplex
	void RebuildClassSet() {};
};

#endif
