#ifndef BASE_ENTITY_INCLUDED
#define BASE_ENTITY_INCLUDED

#include "CStr.h"
#include "ObjectEntry.h"

#include "EntityProperties.h"

class CBaseEntity
{
public:
	// Load from XML
	bool loadXML( CStr filename );

	// Base stats

	CObjectEntry* m_actorObject;

	CStr m_name;
	float m_speed;

	// Extended properties table

	std::hash_map<CStr,CGenericProperty,CStr_hash_compare> m_properties;
};

#endif