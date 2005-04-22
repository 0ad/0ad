// BaseEntity.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity Templates
//
// Usage: These templates are used as the default values for entity properties.
//        Due to Pyrogenesis' data-inheritance model for these properties,
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

#include "scripting/ScriptableComplex.h"
#include "BoundingObjects.h"
#include "EventHandlers.h"
#include "ScriptObject.h"
#include "Xeromyces.h"

class CBaseEntity : public CJSComplex<CBaseEntity>, public IEventTarget
{
public:
	CBaseEntity();
	~CBaseEntity();
	// Load from XML
	bool loadXML( CStr filename );
	// Load a tree of properties from an XML (XMB) node.
	void XMLLoadProperty( const CXeromyces& XeroFile, const XMBElement& Source, CStrW BasePropertyName );

	// Base stats

	CBaseEntity* m_base;
	CStrW m_corpse;
	bool m_extant;

	CStrW m_Base_Name; // <- We don't guarantee the order XML files will be loaded in, so we'll store the name of the
					   //    parent entity referenced, then, after all files are loaded, attempt to match names to objects.

	CStrW m_actorName;
	

	CStrW m_Tag;
	CBoundingCircle* m_bound_circle;
	CBoundingBox* m_bound_box;
	CBoundingObject::EBoundingType m_bound_type;

	float m_speed;
	float m_meleeRange;
	float m_meleeRangeMin;

	float m_turningRadius;
	CScriptObject m_EventHandlers[EVENT_LAST];

	void loadBase();

	// Script-bound functions

	// Get script execution contexts - always run in the context of the entity that fired it.
	JSObject* GetScriptExecContext( IEventTarget* target );

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );

	static void ScriptingInit();
};

#endif
