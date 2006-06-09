#include "precompiled.h"

#include "TechnologyCollection.h"
#include "ps/CLogger.h"
#include "ps/VFSUtil.h"

#define LOG_CATEGORY "tech"

void CTechnologyCollection::LoadFile( const char* path )
{
	//Make tech file reading
	CStrW tag = CStr(path).AfterLast("/").BeforeLast(".xml");
	m_templateFilenames[tag] = path;
}

static void LoadTechThunk( const char* path, const DirEnt* UNUSED(ent), void* context )
{
	CTechnologyCollection* this_ = (CTechnologyCollection*)context;
	this_->LoadFile(path);
}

int CTechnologyCollection::loadTechnologies()
{
	// Load all files in techs/ and subdirectories.
	THROW_ERR( vfs_dir_enum( "technologies/", VFS_DIR_RECURSIVE, "*.xml",
		LoadTechThunk, this ) );
	return 0;
}

CTechnology* CTechnologyCollection::getTechnology( CStrW name )
{
	// Check whether this template has already been loaded.
	//If previously loaded, all slots will be found, so any entry works.
	templateMap::iterator it = m_templates.find( name );
	if( it != m_templates.end() )
		return( it->second );

	// Find the filename corresponding to this template
	templateFilenameMap::iterator filename_it = m_templateFilenames.find( name );
	if( filename_it == m_templateFilenames.end() )
		return( NULL );

	CStr path( filename_it->second );

	//Try to load to the tech
	CTechnology* newTemplate = new CTechnology();
	if( !newTemplate->loadXML( path ) )
	{
		LOG(ERROR, LOG_CATEGORY, "CTechnologyCollection::getTechnology(): Couldn't load template \"%s\"", path.c_str());
		delete newTemplate;
		return( NULL );

	}
	m_templates[name] = newTemplate;
	
	LOG(NORMAL, LOG_CATEGORY, "CTechnologyCollection::getTechnology(): Loaded template \"%s\"", path.c_str());
	return newTemplate;
}

void CTechnologyCollection::getTechnologyNames( std::vector<CStrW>& names )
{
	for( templateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
		if( ! (it->first.Length() > 8 && it->first.Left(8) == L"template"))
			names.push_back( it->first );
}

CTechnologyCollection::~CTechnologyCollection()
{
	for( templateMap::iterator it = m_templates.begin(); it != m_templates.end(); ++it )
		delete( it->second );
}
