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

#ifndef BASE_ENTITY_INCLUDED
#define BASE_ENTITY_INCLUDED

#include "CStr.h"
#include "ObjectEntry.h"

#include "scripting/ScriptableComplex.h"
#include "BoundingObjects.h"
#include "EventHandlers.h"
#include "EntitySupport.h"
#include "ScriptObject.h"
#include "XML/Xeromyces.h"

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

	// The class types this entity has
	SClassSet m_classes;

	CStrW m_Base_Name; // <- We don't guarantee the order XML files will be loaded in, so we'll store the name of the
					   //    parent entity referenced, then, after all files are loaded, attempt to match names to objects.

	CStrW m_actorName;
	
	CStrW m_Tag;
	CBoundingCircle* m_bound_circle;
	CBoundingBox* m_bound_box;
	CBoundingObject::EBoundingType m_bound_type;
	
	//SP properties
	float m_staminaCurr;
	float m_staminaMax;
	float m_staminaBarHeight;
	int m_staminaBarSize;
	float m_staminaBarWidth;

	int m_staminaBorderWidth;
	int m_staminaBorderHeight;
	CStr m_staminaBorderName;

	// HP properties
	float m_healthCurr;
	float m_healthMax;
	float m_healthBarHeight;
	int m_healthBarSize;
	float m_healthBarWidth;

	int m_healthBorderWidth;
	int m_healthBorderHeight;
	CStr m_healthBorderName;
	
	float m_healthRegenRate;
	float m_healthRegenStart;
	float m_healthDecayRate;

	//Rank properties
	float m_rankWidth;
	float m_rankHeight;
	CStr m_rankName;

	// Minimap properties
	CStrW m_minimapType;
	int m_minimapR;
	int m_minimapG;
	int m_minimapB;

	// Y anchor
	CStrW m_anchorType;

	// LOS
	int m_los;
	bool m_permanent;

	// Foundation entity, or "" for none
	CStrW m_foundation;

	float m_speed;
	float m_runRegenRate;
	float m_runDecayRate;

	SEntityAction m_run;
	SEntityAction m_generic;

	int m_sectorDivs;
	int m_pitchDivs;
	float m_anchorConformX;
	float m_anchorConformZ;

	float m_turningRadius;
	CScriptObject m_EventHandlers[EVENT_LAST];

	void loadBase();
	jsval getClassSet();
	void setClassSet( jsval value );
	void rebuildClassSet();

	// Script-bound functions

	// Get script execution contexts - always run in the context of the entity that fired it.
	JSObject* GetScriptExecContext( IEventTarget* target );

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );

	static void ScriptingInit();

private:
	// squelch "unable to generate" warnings
	CBaseEntity(const CBaseEntity& rhs);
	const CBaseEntity& operator=(const CBaseEntity& rhs);

	static STL_HASH_SET<CStr, CStr_hash_compare> scriptsLoaded;
};

#endif
