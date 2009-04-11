//Manages the tech templates.  More detail: see CFormation and CEntityTemplate (collections)


#ifndef INCLUDED_TECHNOLOGYCOLLECTION
#define INCLUDED_TECHNOLOGYCOLLECTION

#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "Technology.h"
#include "ps/Game.h"
#include "ps/Filesystem.h"

#define g_TechnologyCollection CTechnologyCollection::GetSingleton()

class CTechnologyCollection : public Singleton<CTechnologyCollection>
{
	typedef std::map<CStrW, CTechnology*> TechMap;
	typedef std::map<CStrW, CStr> TechFilenameMap;

	TechMap m_techs[PS_MAX_PLAYERS+1];
	TechFilenameMap m_techFilenames;

public:
	std::vector<CTechnology*> activeTechs[PS_MAX_PLAYERS+1];

	CTechnology* GetTechnology( const CStrW& techType, CPlayer* player );
	~CTechnologyCollection();

	int LoadTechnologies();

	// called by non-member trampoline via LoadTechnologies
	void LoadFile( const VfsPath& path );
};

#endif
