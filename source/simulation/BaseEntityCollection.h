// BaseEntityCollection.h
//
// Last modified: 22 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Keeps tabs on the various types of entity that roam the world.
//
// General note: Template, Base Entity, and Entity Class are used more-or-less interchangably.
// 
// Usage: g_EntityTemplateCollection.loadTemplates(): initializes the lists from mods/official/entities/templates
//        After that, you can:
//			Find an entity class by the actor it uses: g_EntityTemplateCollection.getTemplateByActor()
//				Note that this is included solely for loading ScnEd 4,5 and 6 format map files. Don't rely on this
//				working all the time.
//	        Find an entity class by it's name: g_EntityTemplateCollection.getTemplate()
//		  g_EntityManager will also use this class to lookup entity templates when you instantiate an entity with 
//			a class name string.

#ifndef BASEENT_COLLECTION_INCLUDED
#define BASEENT_COLLECTION_INCLUDED

#include <vector>
#include "CStr.h"
#include "Singleton.h"
#include "ObjectEntry.h"
#include "BaseEntity.h"

#define g_EntityTemplateCollection CBaseEntityCollection::GetSingleton()

class CBaseEntityCollection : public Singleton<CBaseEntityCollection>
{
	std::vector<CBaseEntity> m_templates;
public:
	CBaseEntity* getTemplate( CStr entityType );
	void loadTemplates();
	void addTemplate( CBaseEntity& temp );
	CBaseEntity* getTemplateByActor( CObjectEntry* actor );
};

#endif