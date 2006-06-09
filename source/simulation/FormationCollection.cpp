#include "precompiled.h"

#include "FormationCollection.h"
#include "graphics/ObjectManager.h"
#include "graphics/Model.h"
#include "ps/CLogger.h"
#include "ps/VFSUtil.h"

#define LOG_CATEGORY "formation"


void CFormationCollection::LoadFile( const char* path )
{
	// Build the formation name -> filename mapping. This is done so that
	// the formation 'x' can be in units/x.xml, structures/x.xml, etc, and
	// we don't have to search every directory for x.xml.

	// Extract the filename out of the path+name+extension.
	// Equivalent to /.*\/(.*)\.xml/, but not as pretty. 
	CStrW tag = CStr(path).AfterLast("/").BeforeLast(".xml");
	m_templateFilenames[tag] = path;
}

static void LoadFormationThunk( const char* path, const DirEnt* UNUSED(ent), void* context )
{
	CFormationCollection* this_ = (CFormationCollection*)context;
	this_->LoadFile(path);
}

int CFormationCollection::loadTemplates()
{
	// Load all files in formations and subdirectories.
	THROW_ERR( vfs_dir_enum( "formations", VFS_DIR_RECURSIVE, "*.xml",
		LoadFormationThunk, this ) );
	return 0;
}

CFormation* CFormationCollection::getTemplate( CStrW name )
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
	if( !newTemplate->loadXML( path ) )
	{
		LOG(ERROR, LOG_CATEGORY, "CFormationCollection::loadTemplates(): Couldn't load template \"%s\"", path.c_str());
		delete newTemplate;
		return( NULL );
	}

	LOG(NORMAL, LOG_CATEGORY, "CFormationCollection::loadTemplates(): Loaded template \"%s\"", path.c_str());
	m_templates[name] = newTemplate;

	return newTemplate;
}

void CFormationCollection::getFormationNames( std::vector<CStrW>& names )
{
	for( templateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
		if( ! (it->first.Length() > 8 && it->first.Left(8) == L"template"))
			names.push_back( it->first );
}

CFormationCollection::~CFormationCollection()
{
	for( templateMap::iterator it = m_templates.begin(); it != m_templates.end(); ++it )
		delete( it->second );
}
