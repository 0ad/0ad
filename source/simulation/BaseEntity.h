// BaseEntity.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
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

#include "scripting/ScriptableObject.h"
#include "BoundingObjects.h"
#include "EventHandlers.h"
#include "ScriptObject.h"
#include "Xeromyces.h"

class CBaseEntity : public CJSObject<CBaseEntity>
{
public:
	CBaseEntity();
	~CBaseEntity();
	// Load from XML
	bool loadXML( CStr filename );
	// Load a tree of properties from an XML (XMB) node.
	// MT: XeroFile/Source seem to me like they should be const, but the functions they require aren't const members
	// and I'm reluctant to go messing round with xerophilic fungi I don't understand.
	void XMLLoadProperty( CXeromyces& XeroFile, XMBElement& Source, CStrW BasePropertyName );

	// Base stats

	CBaseEntity* m_base;
	CStrW m_Base_Name; // <- We don't guarantee the order XML files will be loaded in, so we'll store the name of the
					   //    parent entity referenced, then, after all files are loaded, attempt to match names to objects.

	CObjectEntry* m_actorObject;

	CStrW m_Tag;
	CBoundingCircle* m_bound_circle;
	CBoundingBox* m_bound_box;
	CBoundingObject::EBoundingType m_bound_type;

	float m_speed;
	float m_turningRadius;
	CScriptObject m_EventHandlers[EVENT_LAST];

	void loadBase();

	// Script-bound functions

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );

	static void ScriptingInit();
};

#endif
