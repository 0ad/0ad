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

//Manages the tech templates.  More detail: see CFormation and CEntityTemplate (collections)


#ifndef INCLUDED_TECHNOLOGYCOLLECTION
#define INCLUDED_TECHNOLOGYCOLLECTION

#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "Technology.h"
#include "ps/Game.h"
#include "ps/Filesystem.h"

#define g_TechnologyCollection CTechnologyCollection::GetSingleton()

class CTechnologyCollection : public Singleton<CTechnologyCollection>
{
	typedef std::map<CStrW, CTechnology*> TechMap;
	typedef std::map<CStrW, VfsPath> TechFilenameMap;

	TechMap m_techs[PS_MAX_PLAYERS+1];
	TechFilenameMap m_techFilenames;

public:
	std::vector<CTechnology*> activeTechs[PS_MAX_PLAYERS+1];

	CTechnology* GetTechnology( const CStrW& techType, CPlayer* player );
	~CTechnologyCollection();

	int LoadTechnologies();

	// called by non-member trampoline via LoadTechnologies
	void LoadFile( const VfsPath& path );
};

#endif
