// BaseEntityCollection.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Keeps tabs on the various types of entity that roam the world.
//
// General note: Template, Base Entity, and Entity Class are used more-or-less interchangably.
// 
// Usage: g_EntityTemplateCollection.loadTemplates(): initializes the lists from entities/templates
//        After that, you can:
//			Find an entity class by the actor it uses: g_EntityTemplateCollection.getTemplateByActor()
//				Note that this is included solely for loading ScnEd 4,5 and 6 format map files. Don't rely on this
//				working all the time.
//	        Find an entity class by its name: g_EntityTemplateCollection.getTemplate()
//		  g_EntityManager will also use this class to lookup entity templates when you instantiate an entity with 
//			a class name string.

#ifndef BASEENT_COLLECTION_INCLUDED
#define BASEENT_COLLECTION_INCLUDED

#include <vector>
#include <map>
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "graphics/ObjectEntry.h"
#include "BaseEntity.h"
#include "ps/Game.h"

#define g_EntityTemplateCollection CBaseEntityCollection::GetSingleton()
#define NULL_PLAYER (PS_MAX_PLAYERS+1)

class CPlayer;

class CBaseEntityCollection : public Singleton<CBaseEntityCollection>
{
	typedef STL_HASH_MAP<CStrW, CBaseEntity*, CStrW_hash_compare> templateMap;
	typedef STL_HASH_MAP<CStrW, CStr, CStrW_hash_compare> templateFilenameMap;
	
	templateMap m_templates[PS_MAX_PLAYERS + 2];
	templateFilenameMap m_templateFilenames;
public:
	~CBaseEntityCollection();
	CBaseEntity* getTemplate( CStrW entityType, CPlayer* player = 0 );
	int loadTemplates();
	void LoadFile( const char* path );

	// Create a list of the names of all base entities, excluding template_*,
	// for display in ScEd's entity-selection box.
	void getBaseEntityNames( std::vector<CStrW>& names );
};

#endif
