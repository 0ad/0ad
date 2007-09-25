#include "precompiled.h"

#include "TechnologyCollection.h"
#include "ps/CLogger.h"
#include "ps/VFSUtil.h"
#include "ps/Player.h"

#define LOG_CATEGORY "tech"

void CTechnologyCollection::LoadFile( const char* path )
{
	//Make tech file reading
	CStrW tag = CStr(path).AfterLast("/").BeforeLast(".xml");
	m_techFilenames[tag] = path;
}

static void LoadTechThunk( const char* path, const DirEnt* UNUSED(ent), uintptr_t cbData )
{
	CTechnologyCollection* this_ = (CTechnologyCollection*)cbData;
	this_->LoadFile(path);
}

int CTechnologyCollection::LoadTechnologies()
{
	// Load all files in techs/ and subdirectories.
	THROW_ERR( vfs_dir_enum( "technologies/", VFS_DIR_RECURSIVE, "*.xml",
		LoadTechThunk, (uintptr_t)this ) );
	return 0;
}

CTechnology* CTechnologyCollection::GetTechnology( const CStrW& name, CPlayer* player )
{
	// Find player ID
	debug_assert( player != 0 );
	int id = player->GetPlayerID();

	// Check whether this template has already been loaded.
	//If previously loaded, all slots will be found, so any entry works.
	TechMap::iterator it = m_techs[id].find( name );
	if( it != m_techs[id].end() )
		return( it->second );

	// Find the filename corresponding to this template
	TechFilenameMap::iterator filename_it = m_techFilenames.find( name );
	if( filename_it == m_techFilenames.end() )
		return( NULL );

	CStr path( filename_it->second );

	//Try to load to the tech
	CTechnology* newTemplate = new CTechnology( name, player );
	if( !newTemplate->LoadXml( path ) )
	{
		LOG(ERROR, LOG_CATEGORY, "CTechnologyCollection::GetTechnology(): Couldn't load tech \"%s\"", path.c_str());
		delete newTemplate;
		return( NULL );

	}
	m_techs[id][name] = newTemplate;
	
	LOG(NORMAL, LOG_CATEGORY, "CTechnologyCollection::GetTechnology(): Loaded tech \"%s\"", path.c_str());
	return newTemplate;
}

CTechnologyCollection::~CTechnologyCollection()
{
	for( uint id=0; id<PS_MAX_PLAYERS+1; id++ )
		for( TechMap::iterator it = m_techs[id].begin(); it != m_techs[id].end(); ++it )
			delete( it->second );
}
