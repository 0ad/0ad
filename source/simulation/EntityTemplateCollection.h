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

// EntityTemplateCollection.h
// 
// Keeps tabs on the various types of entity that roam the world.
//
// General note: Template, Base Entity, and Entity Class are used more-or-less interchangably.
// 
// Usage: g_EntityTemplateCollection.LoadTemplates(): loads all templates
//        g_EntityTemplateCollection.GetTemplate(name): get a template by name
//
// EntityTemplateCollection will look at all subdirectiroes of entities/, but each template's
// name will only be its filename; thus, no two templates should have the same filename,
// but subdirectories can be created in entities/ to organize the files nicely.

#ifndef INCLUDED_ENTITYTEMPLATECOLLECTION
#define INCLUDED_ENTITYTEMPLATECOLLECTION

#include <vector>
#include <map>
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "ps/Game.h"
#include "ps/Filesystem.h"

#define g_EntityTemplateCollection CEntityTemplateCollection::GetSingleton()

class CPlayer;
class CEntityTemplate;

class CEntityTemplateCollection : public Singleton<CEntityTemplateCollection>
{
	// TODO: PS_MAX_PLAYERS doesn't seem to be an upper limit -
	//  "This may be overridden by system.cfg ("max_players")"
	// - so we shouldn't use it here
	static const size_t NULL_PLAYER = (PS_MAX_PLAYERS+1);

	typedef STL_HASH_MAP<CStrW, CEntityTemplate*, CStrW_hash_compare> TemplateMap;
	typedef STL_HASH_MAP<CStrW, VfsPath, CStrW_hash_compare> TemplateFilenameMap;
	
	TemplateMap m_templates[PS_MAX_PLAYERS + 2];
	TemplateFilenameMap m_templateFilenames;
public:
	~CEntityTemplateCollection();
	CEntityTemplate* GetTemplate( const CStrW& entityType, CPlayer* player = 0 );

	// Load list of template filenames
	int LoadTemplates();
	void LoadFile( const VfsPath& path );

	// Create a list of the names of all base entities, excluding template_*,
	// for display in ScEd's entity-selection box.
	void GetEntityTemplateNames( std::vector<CStrW>& names );

	// Get all the templates owned by a specific player, which is useful for techs
	void GetPlayerTemplates( CPlayer* player, std::vector<CEntityTemplate*>& dest );
};

#endif
