#include "precompiled.h"

#include "BaseEntityCollection.h"
#include "ObjectManager.h"
#include "Model.h"
#include "CLogger.h"

#define LOG_CATEGORY "entity"

void CBaseEntityCollection::loadTemplates()
{
	Handle handle;
	vfsDirEnt type, file;
	
	CStr basepath = "entities/";
	CStr pathname;

	// Iterate through all subdirectories 

	Handle maindir = vfs_open_dir( basepath );

	if( maindir > 0 )
	{
		while( !vfs_next_dirent( maindir, &type, "/" ) )
		{
			pathname = basepath + type.name;

			handle = vfs_open_dir( pathname );

			pathname += "/";

			if( handle > 0 )
			{
				while( !vfs_next_dirent(handle, &file, ".xml") )
				{
					CBaseEntity* newTemplate = new CBaseEntity();
					if( newTemplate->loadXML( pathname + file.name ) )
					{
						addTemplate( newTemplate );
						LOG(NORMAL, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Loaded template \"%s%s\"", pathname.c_str(), file.name);
					}
					else
						LOG(ERROR, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Couldn't load template \"%s%s\"", pathname.c_str(), file.name);
				}
				vfs_close_dir( handle );
			}
			else
			{
				LOG(ERROR, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Failed to enumerate entity template directory");
				return;
			}
		}
		vfs_close_dir( maindir );
	}
	else
		LOG(ERROR, LOG_CATEGORY, "CBaseEntityCollection::loadTemplates(): Unable to open directory entities/");	
}

void CBaseEntityCollection::addTemplate( CBaseEntity* temp )
{
	m_templates.push_back( temp );
}

CBaseEntity* CBaseEntityCollection::getTemplate( CStrW name )
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		if( m_templates[t]->m_name == name ) return( m_templates[t] );

	return( NULL );
}

CBaseEntity* CBaseEntityCollection::getTemplateByActor( CObjectEntry* actor )
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		if( m_templates[t]->m_actorObject == actor ) return( m_templates[t] );

	return( NULL );
}

CBaseEntityCollection::~CBaseEntityCollection()
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		delete( m_templates[t] );
}
