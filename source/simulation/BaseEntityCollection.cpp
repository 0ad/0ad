// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "precompiled.h"

#include "BaseEntityCollection.h"
#include "ObjectManager.h"
#include "Model.h"
#include "CLogger.h"

void CBaseEntityCollection::loadTemplates()
{
	Handle handle;
	vfsDirEnt dent;
	
	CStr pathname = "entities/templates/";
	handle=vfs_open_dir(pathname.c_str());
	if (handle > 0)
	{
		while (vfs_next_dirent(handle, &dent, ".xml") == 0)
		{
			CBaseEntity* newTemplate = new CBaseEntity();
			if( newTemplate->loadXML( pathname + dent.name ) )
			{
				addTemplate( newTemplate );
				LOG(NORMAL, "CBaseEntityCollection::loadTemplates(): Loaded template \"%s%s\"", pathname.c_str(), dent.name);
			}
			else
				LOG(ERROR, "CBaseEntityCollection::loadTemplates(): Couldn't load template \"%s%s\"", pathname.c_str(), dent.name);

		}
		vfs_close_dir(handle);
	}
	else
	{
		LOG(ERROR, "CBaseEntityCollection::loadTemplates(): Failed to enumerate entity template directory\n");
		return;
	}
}

void CBaseEntityCollection::addTemplate( CBaseEntity* temp )
{
	m_templates.push_back( temp );
}

CBaseEntity* CBaseEntityCollection::getTemplate( CStr name )
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
