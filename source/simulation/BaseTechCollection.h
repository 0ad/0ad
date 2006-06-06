//Andrew aka pyrolink - ajdecker1022@msn.com
//Manages the tech templates.  More detail: see CBaseFormation and CBaseEntity (collections)


#ifndef BASETECH_COLLECTION_INCLUDED
#define BASETECH_COLLECTION_INCLUDED

#include <vector>
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "BaseTech.h"
#include "ps/Game.h"

#define g_BaseTechCollection CBaseTechCollection::GetSingleton()

class CBaseTechCollection : public Singleton<CBaseTechCollection>
{
	
	typedef std::map<CStrW, CBaseTech*> templateMap;
	typedef std::map<CStrW, CStr> templateFilenameMap;
	templateMap m_templates;
	templateFilenameMap m_templateFilenames;
public:
	~CBaseTechCollection();
	CBaseTech* getTemplate( CStrW techType );
	int loadTemplates();
	void LoadFile( const char* path );

	// Create a list of the names of all base techs, excluding template_*,
	// for display in ScEd's entity-selection box.
	void getBaseTechNames( std::vector<CStrW>& names );
};

#endif
