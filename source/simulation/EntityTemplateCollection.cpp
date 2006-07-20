#include "precompiled.h"

#include "EntityTemplateCollection.h"
#include "graphics/ObjectManager.h"
#include "graphics/Model.h"
#include "ps/CLogger.h"
#include "ps/VFSUtil.h"
#include "ps/Player.h"
#include "ps/Game.h"

#define LOG_CATEGORY "entity"

void CEntityTemplateCollection::LoadFile( const char* path )
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
	CEntityTemplateCollection* this_ = (CEntityTemplateCollection*)context;
	this_->LoadFile(path);
}

int CEntityTemplateCollection::loadTemplates()
{
	// List all files in entities/ and its subdirectories.
	THROW_ERR( vfs_dir_enum( "entities/", VFS_DIR_RECURSIVE, "*.xml",
		LoadFileThunk, this ) );

	/*// Load all the templates; this is necessary so that we can apply techs to them
	// (otherwise a tech can't affect the template of a unit that doesn't yet exist)
	for( TemplateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
	{
		// Load the no-player version of this template (used by techs for base values)
		getTemplate( it->first, 0 );

		for( uint i=0; i<=g_Game->GetNumPlayers(); i++ )
		{
			// TODO: Load the template just once and clone it to get these player templates
			getTemplate( it->first, g_Game->GetPlayer(i) );
		}
	}*/

	return 0;
}

CEntityTemplate* CEntityTemplateCollection::getTemplate( const CStrW& name, CPlayer* player )
{
	// Find player ID
	int id = ( player == 0 ? NULL_PLAYER : player->GetPlayerID() );

	// Check whether this template has already been loaded
	TemplateMap::iterator it = m_templates[id].find( name );
	if( it != m_templates[id].end() )
		return( it->second );

	// Find the filename corresponding to this template
	TemplateFilenameMap::iterator filename_it = m_templateFilenames.find( name );
	if( filename_it == m_templateFilenames.end() )
		return( NULL );

	CStr path( filename_it->second );

	// Try to load to the entity
	CEntityTemplate* newTemplate = new CEntityTemplate( player );
	if( !newTemplate->loadXML( path ) )
	{
		LOG(ERROR, LOG_CATEGORY, "CEntityTemplateCollection::getTemplate(): Couldn't load template \"%s\"", path.c_str());
		delete newTemplate;
		return( NULL );
	}

	LOG(NORMAL, LOG_CATEGORY, "CEntityTemplateCollection::getTemplate(): Loaded template \"%s\"", path.c_str());
	m_templates[id][name] = newTemplate;

	return newTemplate;
}

void CEntityTemplateCollection::getEntityTemplateNames( std::vector<CStrW>& names )
{
	for( TemplateFilenameMap::iterator it = m_templateFilenames.begin(); it != m_templateFilenames.end(); ++it )
		if( ! (it->first.Length() > 8 && it->first.Left(8) == L"template"))
			names.push_back( it->first );
}

void CEntityTemplateCollection::getPlayerTemplates( CPlayer* player, std::vector<CEntityTemplate*>& dest )
{
	int id = ( player == 0 ? NULL_PLAYER : player->GetPlayerID() );

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
