#include "precompiled.h"

#include "BaseEntityCollection.h"
#include "graphics/ObjectManager.h"
#include "graphics/Model.h"
#include "ps/CLogger.h"
#include "ps/VFSUtil.h"
#include "ps/Player.h"

#define LOG_CATEGORY "entity"

void CBaseEntityCollection::LoadFile( const char* path )
{
	// Build the entity name -> filename mapping. This is done so that
	// the entity 'x' can be in units/x.xml, structures/x.xml, etc, and
	// we don't have to search every directory for x.xml.

	// Extract the filename out of the path+name+extension.
	// Equivalent to /.*\/(.*)\.xml/, but not as pretty. 
	CStrW tag = CStr(path).AfterLast("/").BeforeLast(".xml");
	m_templateFilenames[tag] = path;
}

static void LoadFileThunk( const char* path, const DirEnt* UNUSED(ent), void* context )
{
	CBaseEntityCollection* this_ = (CBaseEntityCollection*)context;
	this_->LoadFile(path);
}

int CBaseEntityCollection::loadTemplates()
{
	// Load all files in entities/ and its subdirectories.
	THROW_ERR( vfs_dir_enum( "entities/", VFS_DIR_RECURSIVE, "*.xml",
		LoadFileThunk, this ) );
	return 0;
}

CBaseEntity* CBaseEntityCollection::getTemplate( CStrW name, CPlayer* player )
{
	// Find player ID
	int id = ( player == 0 ? NULL_PLAYER : player->GetPlayerID() );

	// Check whether this template has already been loaded
	templateMap::iterator it = m_templates[id].find( name );
	if( it != m_templates[id].end() )
		return( it->second );

	// Find the filename corresponding to this template
	templateFilenameMap::iterator filename_it = m_templateFilenames.find( name );
	if( filename_it == m_templateFilenames.end() )
		return( NULL );

	CStr path( filename_it->second );

	// Try to load to the entity
	CBaseEntity* newTemplate = new CBaseEntity( player );
	if( !newTemplate->loadXML( path ) )
	{
		LOG(ERROR, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Couldn't load template \"%s\"", path.c_str());
		delete newTemplate;
		return( NULL );
	}

	LOG(NORMAL, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Loaded template \"%s\"", path.c_str());
	m_templates[id][name] = newTemplate;

	return newTemplate;
}

void CBaseEntityCollection::getBaseEntityNames( std::vector<CStrW>& names )
{
	for( templateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
		if( ! (it->first.Length() > 8 && it->first.Left(8) == L"template"))
			names.push_back( it->first );
}

CBaseEntityCollection::~CBaseEntityCollection()
{
	for( int id = 0; id < PS_MAX_PLAYERS + 2; id++ )
		for( templateMap::iterator it = m_templates[id].begin(); it != m_templates[id].end(); ++it )
			delete( it->second );
}
