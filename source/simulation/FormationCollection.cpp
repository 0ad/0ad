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

#include "FormationCollection.h"
#include "graphics/ObjectManager.h"
#include "graphics/Model.h"
#include "ps/CLogger.h"

#define LOG_CATEGORY "formation"


void CFormationCollection::LoadFile( const VfsPath& pathname )
{
	// Build the formation name -> filename mapping. This is done so that
	// the formation 'x' can be in units/x.xml, structures/x.xml, etc, and
	// we don't have to search every directory for x.xml.

	const CStrW basename(fs::basename(pathname));
	m_templateFilenames[basename] = pathname.string();
}

static LibError LoadFormationThunk( const VfsPath& path, const FileInfo& UNUSED(fileInfo), uintptr_t cbData )
{
	CFormationCollection* this_ = (CFormationCollection*)cbData;
	this_->LoadFile(path);
	return INFO::CB_CONTINUE;
}

int CFormationCollection::LoadTemplates()
{
	// Load all files in formations and subdirectories.
	THROW_ERR( fs_util::ForEachFile(g_VFS, "formations/", LoadFormationThunk, (uintptr_t)this, "*.xml", fs_util::DIR_RECURSIVE));
	return 0;
}

CFormation* CFormationCollection::GetTemplate( const CStrW& name )
{
	// Check whether this template has already been loaded
	templateMap::iterator it = m_templates.find( name );
	if( it != m_templates.end() )
		return( it->second );

	// Find the filename corresponding to this template
	templateFilenameMap::iterator filename_it = m_templateFilenames.find( name );
	if( filename_it == m_templateFilenames.end() )
		return( NULL );

	CStr path( filename_it->second );

	//Try to load to the formation
	CFormation* newTemplate = new CFormation();
	if( !newTemplate->LoadXml( path ) )
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CFormationCollection::LoadTemplates(): Couldn't load template \"%s\"", path.c_str());
		delete newTemplate;
		return( NULL );
	}

	LOG(CLogger::Normal,  LOG_CATEGORY, "CFormationCollection::LoadTemplates(): Loaded template \"%s\"", path.c_str());
	m_templates[name] = newTemplate;

	return newTemplate;
}

void CFormationCollection::GetFormationNames( std::vector<CStrW>& names )
{
	for( templateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
		if( ! (it->first.length() > 8 && it->first.Left(8) == L"template"))
			names.push_back( it->first );
}

CFormationCollection::~CFormationCollection()
{
	for( templateMap::iterator it = m_templates.begin(); it != m_templates.end(); ++it )
		delete( it->second );
}
