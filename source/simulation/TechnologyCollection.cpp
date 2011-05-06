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

#include "TechnologyCollection.h"
#include "ps/CLogger.h"
#include "ps/Player.h"

#define LOG_CATEGORY L"tech"

void CTechnologyCollection::LoadFile( const VfsPath& pathname )
{
	const CStrW basename(fs::basename(pathname));
	m_techFilenames[basename] = pathname;
}

static LibError LoadTechThunk( const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), uintptr_t cbData )
{
	CTechnologyCollection* this_ = (CTechnologyCollection*)cbData;
	this_->LoadFile(pathname);
	return INFO::CONTINUE;
}

int CTechnologyCollection::LoadTechnologies()
{
	// Load all files in techs/ and subdirectories.
	THROW_ERR( fs_util::ForEachFile(g_VFS, L"technologies/", LoadTechThunk, (uintptr_t)this, L"*.xml", fs_util::DIR_RECURSIVE));
	return 0;
}

CTechnology* CTechnologyCollection::GetTechnology( const CStrW& name, CPlayer* player )
{
	// Find player ID
	debug_assert( player != 0 );
	const size_t id = player->GetPlayerID();

	// Check whether this template has already been loaded.
	//If previously loaded, all slots will be found, so any entry works.
	TechMap::iterator it = m_techs[id].find( name );
	if( it != m_techs[id].end() )
		return( it->second );

	// Find the filename corresponding to this template
	TechFilenameMap::iterator filename_it = m_techFilenames.find( name );
	if( filename_it == m_techFilenames.end() )
		return( NULL );

	VfsPath path( filename_it->second );

	//Try to load to the tech
	CTechnology* newTemplate = new CTechnology( name, player );
	if( !newTemplate->LoadXml( path ) )
	{
		LOG(CLogger::Error, LOG_CATEGORY, L"CTechnologyCollection::GetTechnology(): Couldn't load tech \"%ls\"", path.string().c_str());
		delete newTemplate;
		return( NULL );

	}
	m_techs[id][name] = newTemplate;
	
	LOG(CLogger::Normal,  LOG_CATEGORY, L"CTechnologyCollection::GetTechnology(): Loaded tech \"%ls\"", path.string().c_str());
	return newTemplate;
}

CTechnologyCollection::~CTechnologyCollection()
{
	for( size_t id=0; id<PS_MAX_PLAYERS+1; id++ )
		for( TechMap::iterator it = m_techs[id].begin(); it != m_techs[id].end(); ++it )
			delete( it->second );
}
