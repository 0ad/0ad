//Andrew aka pyrolink - ajdecker1022@msn.com
//Manages the tech templates.  More detail: see CFormation and CEntityTemplate (collections)


#ifndef TECHNOLOGY_COLLECTION_INCLUDED
#define TECHNOLOGY_COLLECTION_INCLUDED

#include <vector>
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "Technology.h"
#include "ps/Game.h"

#define g_TechnologyCollection CTechnologyCollection::GetSingleton()

class CTechnologyCollection : public Singleton<CTechnologyCollection>
{
	
	typedef std::map<CStrW, CTechnology*> templateMap;
	typedef std::map<CStrW, CStr> templateFilenameMap;
	templateMap m_templates;
	templateFilenameMap m_templateFilenames;
public:
	~CTechnologyCollection();
	CTechnology* getTechnology( CStrW techType );
	int loadTechnologies();
	void LoadFile( const char* path );

	// Create a list of the names of all base techs, excluding template_*,
	// for display in ScEd's entity-selection box.
	void getTechnologyNames( std::vector<CStrW>& names );
};

#endif
