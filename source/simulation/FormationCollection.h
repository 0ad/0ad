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

//FormationCollection.h

//Nearly identical to EntityTemplateCollection and associates i.e. EntityTemplate, Entity.
//This is the manager of the entity formation "templates"


#ifndef INCLUDED_FORMATIONCOLLECTION
#define INCLUDED_FORMATIONCOLLECTION

#include <vector>
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "ps/Filesystem.h"
#include "Formation.h"

#define g_EntityFormationCollection CFormationCollection::GetSingleton()


class CFormationCollection : public Singleton<CFormationCollection>
{
	
	typedef std::map<CStrW, CFormation*> templateMap;
	typedef std::map<CStrW, VfsPath> templateFilenameMap;
	templateMap m_templates;
	templateFilenameMap m_templateFilenames;
public:
	~CFormationCollection();
	CFormation* GetTemplate( const CStrW& formationType );
	int LoadTemplates();
	void LoadFile( const VfsPath& path );

	// Create a list of the names of all base entities, excluding template_*,
	// for display in ScEd's entity-selection box.
	void GetFormationNames( std::vector<CStrW>& names );
};

#endif
