// BaseEntity.h
//
// Last modified: 22 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity Templates
//
// Usage: These templates are used as the default values for entity properties.
//        Due to Prometheus' data-inheritance model for these properties,
//			 templates specify a base-template, then override only those properties
//			 in that template that need to change.
//        Similarly, entities need only specify properties they possess that are
//           different to the ones in their respective templates. Of course,
//           properties such as position, current HP, etc. cannot be inherited from
//           a template in this way.
//
//        Note: Data-inheritance is not currently implemented.

#ifndef BASE_ENTITY_INCLUDED
#define BASE_ENTITY_INCLUDED

#include "CStr.h"
#include "ObjectEntry.h"

#include "EntityProperties.h"
#include "BoundingObjects.h"

class CBaseEntity : public IPropertyOwner
{
public:
	CBaseEntity();
	~CBaseEntity();
	// Load from XML
	bool loadXML( CStr filename );

	// Base stats

	CObjectEntry* m_actorObject;

	CProperty_CStr m_name;
	CBoundingCircle* m_bound_circle;
	CBoundingBox* m_bound_box;
	CBoundingObject::EBoundingType m_bound_type;

	CProperty_float m_speed;
	CProperty_float m_turningRadius;
};

#endif
