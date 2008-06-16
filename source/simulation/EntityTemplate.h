// EntityTemplate.h
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

#ifndef INCLUDED_ENTITYTEMPLATE
#define INCLUDED_ENTITYTEMPLATE

#include "ps/CStr.h"

#include "scripting/ScriptableComplex.h"
#include "scripting/DOMEvent.h"
#include "BoundingObjects.h"
#include "EntitySupport.h"
#include "ScriptObject.h"

class CPlayer;
class CXeromyces;
class XMBElement;

class CEntityTemplate : public CJSComplex<CEntityTemplate>, public IEventTarget, boost::noncopyable
{
public:
	CPlayer* m_player;		// Which player this template is for, or null for the no-player template
							// used to read unmodified values for upgrades and such.

	CEntityTemplate(CPlayer* player);
	~CEntityTemplate();
	// Load from XML
	bool LoadXml(const CStr& filename);
	// Load a tree of properties from an XML (XMB) node.
	void XMLLoadProperty(const CXeromyces& XeroFile, const XMBElement& Source, const CStrW& BasePropertyName);

	// Base stats
	CEntityTemplate* m_base;
	//bool m_extant;

	// The unmodified, no-player version of this template
	CEntityTemplate* m_unmodified;

	// The class types this entity has
	CClassSet m_classes;

	CStrW m_Base_Name; // <- We don't guarantee the order XML files will be loaded in, so we'll store the name of the
					   //    parent entity referenced, then, after all files are loaded, attempt to match names to objects.

	CStrW m_actorName;
	
	CStrW m_Tag;
	CBoundingCircle* m_bound_circle;
	CBoundingBox* m_bound_box;
	CBoundingObject::EBoundingType m_bound_type;

	// Sound properties
	// map animation name to soundGroup index returned by SoundGroupMgr
	typedef STL_HASH_MAP<CStr, size_t, CStr_hash_compare> SoundGroupTable;
	SoundGroupTable m_SoundGroupTable;
	
	//SP properties
	float m_staminaMax;

	// HP properties
	float m_healthMax;


	float m_healthRegenRate;
	float m_healthRegenStart;
	float m_healthDecayRate;

	// Display properties

	bool m_barsEnabled;
	float m_barOffset;
	float m_barHeight;
	float m_barWidth;
	float m_barBorderSize;
	CStr m_barBorder;

	float m_staminaBarHeight;
	int m_staminaBarSize;
	float m_staminaBarWidth;
	int m_staminaBorderWidth;
	int m_staminaBorderHeight;
	CStr m_staminaBorderName;

	float m_healthBarHeight;
	int m_healthBarSize;
	float m_healthBarWidth;
	int m_healthBorderWidth;
	int m_healthBorderHeight;
	CStr m_healthBorderName;

	float m_rankWidth;
	float m_rankHeight;
	
	// Stance name
	CStr m_stanceName;

	//Rank properties
	CStr m_rankName;
	//Rally name
	CStr m_rallyName;
	float m_rallyWidth;
	float m_rallyHeight;

	// Minimap properties
	CStrW m_minimapType;
	int m_minimapR;
	int m_minimapG;
	int m_minimapB;

	// Y anchor
	CStrW m_anchorType;

	// LOS
	int m_los;
	bool m_visionPermanent;

	// Is this object a territory centre? (e.g. Settlements in 0AD)
	bool m_isTerritoryCentre;

	// Foundation entity, or "" for none
	CStrW m_foundation;

	// Socket entity that we can be built on, or "" for free placement
	CStrW m_socket;

	// Can be "allied" to allow placement only in allied territories, or "" or "all" for all territories
	CStrW m_territoryRestriction;

	// Territorial limit
	CStrW m_buildingLimitCategory;

	float m_speed;
	float m_runSpeed;
	float m_runRegenRate;
	float m_runDecayRate;
	bool m_passThroughAllies;

	float m_runMaxRange;
	float m_runMinRange;

	int m_sectorDivs;
	int m_pitchDivs;
	float m_pitchValue;
	float m_anchorConformX;
	float m_anchorConformZ;

	float m_turningRadius;
	CScriptObject m_EventHandlers[EVENT_LAST];

	void LoadBase();
	jsval GetClassSet();
	void SetClassSet( jsval value );
	void RebuildClassSet();

	// Script-bound functions

	// Get script execution contexts - always run in the context of the entity that fired it.
	JSObject* GetScriptExecContext(IEventTarget* target);

	CStr ToString(JSContext* cx, uintN argc, jsval* argv);

	static void ScriptingInit();

private:
	static STL_HASH_SET<CStr, CStr_hash_compare> scriptsLoaded;
};

#endif
