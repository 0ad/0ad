// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "BaseEntityCollection.h"
#include "ObjectManager.h"
#include "Model.h"
#include "io.h"

void CBaseEntityCollection::loadTemplates()
{
	_finddata_t file;
	intptr_t handle;

	CStr pathname = "mods/official/entities/templates/";
	CStr filespec = pathname + "*.xml";
	
	handle = _findfirst( filespec, &file );
    if( handle != -1 )
	{	
		do
		{
			CBaseEntity newTemplate;
			if( newTemplate.loadXML( pathname + file.name ) )
				addTemplate( newTemplate );
		}
		while( !_findnext( handle, &file ) );

        _findclose(handle);
	}

	// He's so annoyingly slow...
	CBaseEntity* dude = getTemplate( "Prometheus Dude" );
	dude->m_actorObject->m_WalkAnim->m_FrameTime /= 10.0f;
	dude->m_speed *= 10.0f;

/*

	// Nasty evil wicked hardcoding.
	CBaseEntity dude;
	dude.m_name = CStr( "Prometheus Dude" );
	dude.m_actorObject = g_ObjMan.FindObject( "The Dude" );
	dude.m_actorObject->m_Model->GetAnimation()->m_FrameTime /= 3.0f;
	dude.speed = 0.3f;

	addTemplate( dude );
*/
}

void CBaseEntityCollection::addTemplate( CBaseEntity& temp )
{
	m_templates.push_back( temp );
}

CBaseEntity* CBaseEntityCollection::getTemplate( CStr name )
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		if( m_templates[t].m_name == name ) return( &( m_templates[t] ) );

	return( NULL );
}

CBaseEntity* CBaseEntityCollection::getTemplateByActor( CObjectEntry* actor )
{
	for( u16 t = 0; t < m_templates.size(); t++ )
		if( m_templates[t].m_actorObject == actor ) return( &( m_templates[t] ) );

	return( NULL );
}
