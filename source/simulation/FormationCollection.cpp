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

	const CStrW basename(fs::basename((const fs::path&)pathname));
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
	THROW_ERR( fs_ForEachFile(g_VFS, "formations/", LoadFormationThunk, (uintptr_t)this, "*.xml", DIR_RECURSIVE));
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
