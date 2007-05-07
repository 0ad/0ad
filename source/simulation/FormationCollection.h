//FormationCollection.h

//Nearly identical to EntityTemplateCollection and associates i.e. EntityTemplate, Entity.
//This is the manager of the entity formation "templates"


#ifndef INCLUDED_FORMATIONCOLLECTION
#define INCLUDED_FORMATIONCOLLECTION

#include <vector>
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "Formation.h"

#define g_EntityFormationCollection CFormationCollection::GetSingleton()


class CFormationCollection : public Singleton<CFormationCollection>
{
	
	typedef std::map<CStrW, CFormation*> templateMap;
	typedef std::map<CStrW, CStr> templateFilenameMap;
	templateMap m_templates;
	templateFilenameMap m_templateFilenames;
public:
	~CFormationCollection();
	CFormation* GetTemplate( const CStrW& formationType );
	int LoadTemplates();
	void LoadFile( const char* path );

	// Create a list of the names of all base entities, excluding template_*,
	// for display in ScEd's entity-selection box.
	void GetFormationNames( std::vector<CStrW>& names );
};

#endif
