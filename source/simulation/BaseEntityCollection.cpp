#include "precompiled.h"

#include "BaseEntityCollection.h"
#include "ObjectManager.h"
#include "Model.h"
#include "CLogger.h"
#include "VFSUtil.h"

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
	THROW_ERR( VFSUtil::EnumDirEnts( "entities/", VFSUtil::RECURSIVE, "*.xml",
		LoadFileThunk, this ) );
	return 0;
}

CBaseEntity* CBaseEntityCollection::getTemplate( CStrW name )
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

	// Try to load to the entity
	CBaseEntity* newTemplate = new CBaseEntity();
	if( !newTemplate->loadXML( path ) )
	{
		LOG(ERROR, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Couldn't load template \"%s\"", path.c_str());
		return( NULL );
	}

	LOG(NORMAL, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Loaded template \"%s\"", path.c_str());
	m_templates[name] = newTemplate;

	// Load the entity's parent, if it has one
	if( newTemplate->m_Base_Name.Length() )
	{
		CBaseEntity* base = getTemplate( newTemplate->m_Base_Name );
		if( base )
		{
			newTemplate->m_base = base;
			newTemplate->loadBase();
		}
		else
		{
			LOG( WARNING, LOG_CATEGORY, "Parent template \"%ls\" does not exist in template \"%ls\"", newTemplate->m_Base_Name.c_str(), newTemplate->m_Tag.c_str() );
			// (The requested entity will still be returned, but with no parent.
			// Is this a reasonable thing to do?)
		}
	}

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
	for( templateMap::iterator it = m_templates.begin(); it != m_templates.end(); ++it )
		delete( it->second );
}
