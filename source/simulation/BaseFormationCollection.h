//BaseFormationCollection.h
//Andrew aka pyrolink:  ajdecker1022@msn.com

//Nearly identical to BaseEntityCollection and associates i.e. BaseEntity, Entity.
//This is the manager of the entity formation "templates"


#ifndef BASEFORM_COLLECTION_INCLUDED
#define BASEFORM_COLLECTION_INCLUDED

#include <vector>
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "BaseFormation.h"

#define g_EntityFormationCollection CBaseFormationCollection::GetSingleton()


class CBaseFormationCollection : public Singleton<CBaseFormationCollection>
{
	
	typedef std::map<CStrW, CBaseFormation*> templateMap;
	typedef std::map<CStrW, CStr> templateFilenameMap;
	templateMap m_templates;
	templateFilenameMap m_templateFilenames;
public:
	~CBaseFormationCollection();
	CBaseFormation* getTemplate( CStrW formationType );
	int loadTemplates();
	void LoadFile( const char* path );

	// Create a list of the names of all base entities, excluding template_*,
	// for display in ScEd's entity-selection box.
	void getBaseFormationNames( std::vector<CStrW>& names );
};

#endif
