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
#include "CStr.h"
#include "Singleton.h"
#include "ObjectEntry.h"
#include "BaseEntity.h"

#define g_EntityTemplateCollection CBaseEntityCollection::GetSingleton()

class CBaseEntityCollection : public Singleton<CBaseEntityCollection>
{
	typedef std::map<CStrW, CBaseEntity*> templateMap;
	typedef std::map<CStrW, CStr> templateFilenameMap;
	templateMap m_templates;
	templateFilenameMap m_templateFilenames;
public:
	~CBaseEntityCollection();
	CBaseEntity* getTemplate( CStrW entityType );
	void loadTemplates();
	void LoadFile( const char* path );

	// Create a list of the names of all templates, for display in ScEd's
	// entity-selection box. (This isn't really very good, since it includes
	// 'abstract' entity classes that should never be created, and it doesn't
	// split them into useful categories or give them readable names.)
	void getTemplateNames( std::vector<CStrW>& names );
#ifdef SCED // a slightly unpleasant hack, since ScEd can only remember numbers:
	CBaseEntity* getTemplateByID( int n );
#endif
};

#endif
