#include "precompiled.h"

#include "BaseEntityCollection.h"
#include "ObjectManager.h"
#include "Model.h"
#include "CLogger.h"
#include "VFSUtil.h"

#define LOG_CATEGORY "entity"


void CBaseEntityCollection::LoadFile( const char* path )
{
	CBaseEntity* newTemplate = new CBaseEntity();
	if( newTemplate->loadXML( path ) )
	{
		m_templates.push_back( newTemplate );
		LOG(NORMAL, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Loaded template \"%s\"", path);
	}
	else
		LOG(ERROR, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Couldn't load template \"%s\"", path);
}

static void LoadFileThunk( const char* path, const vfsDirEnt* ent, void* context )
{
	CBaseEntityCollection* this_ = (CBaseEntityCollection*)context;
	this_->LoadFile(path);
}

void CBaseEntityCollection::loadTemplates()
{
	// Load all files in entities/ and its subdirectories.
	THROW_ERR( VFSUtil::EnumDirEnts( "entities/", "*.xml", true, LoadFileThunk, this ) );


	// Fix up parent links in the templates.
	
	std::vector<CBaseEntity*>::iterator it, it_done;
	std::vector<CBaseEntity*> done;

	// TODO: MT: Circular references check.

	while( done.size() < m_templates.size() )
	{
		for( it = m_templates.begin(); it != m_templates.end(); it++ )
		{
			if( !( (*it)->m_Base_Name.Length() ) )
			{
				done.push_back( *it );
				continue;
			}

			CBaseEntity* Base = getTemplate( (*it)->m_Base_Name );
			if( Base )
			{
				// Check whether it's been loaded yet.
				for( it_done = done.begin(); it_done != done.end(); it_done++ )
				{
					if( *it_done == Base )
					{
						(*it)->m_base = Base;
						(*it)->loadBase();
						Base = NULL;
						break;
					}
				}
				if( !Base )
				{
					// Done
					done.push_back( *it );
					continue;
				}
			}
			else
				LOG( WARNING, LOG_CATEGORY, "Parent template %s does not exist in template %s", CStr8( (*it)->m_Base_Name ).c_str(), CStr8( (*it)->m_Tag ).c_str() );
		}
	}
}

CBaseEntity* CBaseEntityCollection::getTemplate( CStrW name )
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		if( m_templates[t]->m_Tag == name ) return( m_templates[t] );

	return( NULL );
}

CBaseEntity* CBaseEntityCollection::getTemplateByActor( CStrW actorName )
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		if( m_templates[t]->m_actorName == actorName ) return( m_templates[t] );

	return( NULL );
}

void CBaseEntityCollection::getTemplateNames( std::vector<CStrW>& names )
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		names.push_back( m_templates[t]->m_Tag );
}

#ifdef SCED
CBaseEntity* CBaseEntityCollection::getTemplateByID( int n )
{
	return m_templates[n];
}
#endif

CBaseEntityCollection::~CBaseEntityCollection()
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		delete( m_templates[t] );
}
