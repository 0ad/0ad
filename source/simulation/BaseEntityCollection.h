// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

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