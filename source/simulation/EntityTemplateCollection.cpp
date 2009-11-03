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

#include "precompiled.h"

#include "EntityTemplateCollection.h"
#include "EntityTemplate.h"
#include "graphics/ObjectManager.h"
#include "graphics/Model.h"
#include "ps/CLogger.h"
#include "ps/Player.h"
#include "ps/Game.h"

#define LOG_CATEGORY L"entity"


#include <boost/filesystem.hpp>

void CEntityTemplateCollection::LoadFile( const VfsPath& pathname )
{
	// Build the entity name -> filename mapping. This is done so that
	// the entity 'x' can be in units/x.xml, structures/x.xml, etc, and
	// we don't have to search every directory for x.xml.

	const CStrW basename(fs::basename(pathname));
	m_templateFilenames[basename] = pathname;
}

static LibError LoadFileThunk( const VfsPath& path, const FileInfo& UNUSED(fileInfo), uintptr_t cbData )
{
	CEntityTemplateCollection* this_ = (CEntityTemplateCollection*)cbData;
	this_->LoadFile(path);
	return INFO::CB_CONTINUE;
}

int CEntityTemplateCollection::LoadTemplates()
{
	// List all files in entities/ and its subdirectories.
	THROW_ERR( fs_util::ForEachFile(g_VFS, L"entities/", LoadFileThunk, (uintptr_t)this, L"*.xml", fs_util::DIR_RECURSIVE));

	/*// Load all the templates; this is necessary so that we can apply techs to them
	// (otherwise a tech can't affect the template of a unit that doesn't yet exist)
	for( TemplateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
	{
		// Load the no-player version of this template (used by techs for base values)
		GetTemplate( it->first, 0 );

		for( size_t i=0; i<=g_Game->GetNumPlayers(); i++ )
		{
			// TODO: Load the template just once and clone it to get these player templates
			GetTemplate( it->first, g_Game->GetPlayer(i) );
		}
	}*/

	return 0;
}

CEntityTemplate* CEntityTemplateCollection::GetTemplate( const CStrW& name, CPlayer* player )
{
	// Find player ID
	size_t id = ( player == 0 ? NULL_PLAYER : player->GetPlayerID() );

	// Check whether this template has already been loaded
	TemplateMap::iterator it = m_templates[id].find( name );
	if( it != m_templates[id].end() )
		return( it->second );

	// Find the filename corresponding to this template
	TemplateFilenameMap::iterator filename_it = m_templateFilenames.find( name );
	if( filename_it == m_templateFilenames.end() )
		return( NULL );

	VfsPath path( filename_it->second );

	// Try to load to the entity
	CEntityTemplate* newTemplate = new CEntityTemplate( player );
	if( !newTemplate->LoadXml( path ) )
	{
		LOG(CLogger::Error, LOG_CATEGORY, L"CEntityTemplateCollection::GetTemplate(): Couldn't load template \"%ls\"", path.string().c_str());
		delete newTemplate;
		return( NULL );
	}

	LOG(CLogger::Normal,  LOG_CATEGORY, L"CEntityTemplateCollection::GetTemplate(): Loaded template \"%ls\"", path.string().c_str());
	m_templates[id][name] = newTemplate;

	return newTemplate;
}

void CEntityTemplateCollection::GetEntityTemplateNames( std::vector<CStrW>& names )
{
	for( TemplateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
		if( ! (it->first.length() > 8 && it->first.Left(8) == L"template"))
			names.push_back( it->first );
}

void CEntityTemplateCollection::GetPlayerTemplates( CPlayer* player, std::vector<CEntityTemplate*>& dest )
{
	size_t id = ( player == 0 ? NULL_PLAYER : player->GetPlayerID() );

	for( TemplateMap::iterator it = m_templates[id].begin(); it != m_templates[id].end(); ++it )
	{
		dest.push_back( it->second );
	}
}

CEntityTemplateCollection::~CEntityTemplateCollection()
{
	for( int id = 0; id < PS_MAX_PLAYERS + 2; id++ )
		for( TemplateMap::iterator it = m_templates[id].begin(); it != m_templates[id].end(); ++it )
			delete( it->second );
}
