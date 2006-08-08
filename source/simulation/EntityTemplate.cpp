#include "precompiled.h"

#include "EntityTemplate.h"
#include "EntityTemplateCollection.h"
#include "graphics/ObjectManager.h"
#include "ps/CStr.h"
#include "ps/Player.h"
#include "scripting/ScriptableComplex.inl"
#include "ps/XML/Xeromyces.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "entity"

STL_HASH_SET<CStr, CStr_hash_compare> CEntityTemplate::scriptsLoaded;

CEntityTemplate::CEntityTemplate( CPlayer* player )
{
	m_player = player;
	m_base = NULL;
	
	AddProperty( L"tag", &m_Tag, false );
	AddProperty( L"parent", &m_base, false );
	AddProperty( L"unmodified", &m_unmodified, false );

	for( int t = 0; t < EVENT_LAST; t++ )
	{
		AddProperty( EventNames[t], &m_EventHandlers[t], false );
		AddHandler( t, &m_EventHandlers[t] );
	}

	// Initialize, make life a little easier on the scriptors
	m_speed = m_turningRadius = 0.0f;
	//m_extant = true; 
	m_foundation = CStrW();
	m_socket = CStrW();
	m_passThroughAllies = false;
	m_sectorDivs = 4;

	// Sentinel values for stamina and health (so scripts can check if an entity has no stamina or no HP).
	m_speed = 0;
	m_staminaMax = 0;
	m_healthMax = 0;

	m_bound_type = CBoundingObject::BOUND_NONE;
	m_bound_circle = NULL;
	m_bound_box = NULL;

	// If these aren't set, we can get an infinite loop in CEntity::interpolate, which is nasty; they
	// should be set in template_entity, but do this in case some entity forgets to inherit from that
	m_anchorConformX = 0;
	m_anchorConformZ = 0;
}

CEntityTemplate::~CEntityTemplate()
{
	if( m_bound_box )
		delete( m_bound_box );
	if( m_bound_circle )
		delete( m_bound_circle );
}

void CEntityTemplate::loadBase()
{ 
	// Don't bother if we're providing a replacement.
	if( m_bound_type == CBoundingObject::BOUND_NONE )
	{
		if( m_base->m_bound_type == CBoundingObject::BOUND_CIRCLE )
		{
 			m_bound_circle = new CBoundingCircle();
			m_bound_circle->setRadius( m_base->m_bound_circle->m_radius );
			m_bound_circle->setHeight( m_base->m_bound_circle->m_height );
		}
		else if( m_base->m_bound_type == CBoundingObject::BOUND_OABB )
		{
			m_bound_box = new CBoundingBox();
			m_bound_box->setDimensions( m_base->m_bound_box->getWidth(), m_base->m_bound_box->getDepth() );
			m_bound_box->setHeight( m_base->m_bound_box->m_height );
		}
		m_bound_type = m_base->m_bound_type;
	}

	SetBase( m_base );
	m_classes.SetParent( &( m_base->m_classes ) );
	SetNextObject( m_base );
}

jsval CEntityTemplate::getClassSet()
{
	CStrW result = m_classes.getMemberList(); 
	return( ToJSVal( result ) );
}

void CEntityTemplate::setClassSet( jsval value )
{
	CStr memberCmdList = ToPrimitive<CStrW>( value );
	m_classes.setFromMemberList(memberCmdList);

	rebuildClassSet();
}

void CEntityTemplate::rebuildClassSet()
{
	m_classes.Rebuild();
	InheritorsList::iterator it;
	for( it = m_Inheritors.begin(); it != m_Inheritors.end(); it++ )
		(*it)->rebuildClassSet();
}

bool CEntityTemplate::loadXML( const CStr& filename )
{
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		// Fail
		return false;

	// Define all the elements and attributes used in the XML file
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	#define AT(x) int at_##x = XeroFile.getAttributeID(#x)
	// Only the ones we can't load using normal methods.
	EL(Entity);
	EL(Script);
	EL(Event);
	EL(Traits);
	EL(Footprint);
	EL(Depth);
	EL(Height);
	EL(Radius);
	EL(Width);
	AT(Parent);
	AT(On);
	AT(File);
	AT(Function);
	#undef AT
	#undef EL

	XMBElement Root = XeroFile.getRoot();

	if( Root.getNodeName() != el_Entity )
	{
		LOG( ERROR, LOG_CATEGORY, "CEntityTemplate::LoadXML: XML root was not \"Entity\" in file %s. Load failed.", filename.c_str() );
		return( false );
	}

	XMBElementList RootChildren = Root.getChildNodes();

	m_Tag = CStr(filename).AfterLast("/").BeforeLast(".xml");

	m_Base_Name = Root.getAttributes().getNamedItem( at_Parent );

	// Load our parent, if we have one
	if( m_Base_Name.Length() )
	{
		CEntityTemplate* base = g_EntityTemplateCollection.getTemplate( m_Base_Name, m_player );
		if( base )
		{
			m_base = base;
			loadBase();
		}
		else
		{
			LOG( WARNING, LOG_CATEGORY, "Parent template \"%ls\" does not exist in template \"%ls\"", m_Base_Name.c_str(), m_Tag.c_str() );
			// (The requested entity will still be returned, but with no parent.
			// Is this a reasonable thing to do?)
		}
	}
	
	for (int i = 0; i < RootChildren.Count; ++i)
	{
		XMBElement Child = RootChildren.item(i);

		int ChildName = Child.getNodeName();
		if( ChildName == el_Script )
		{
			CStr Include = Child.getAttributes().getNamedItem( at_File );

			if( Include.Length() && scriptsLoaded.find( Include ) == scriptsLoaded.end() )
			{
				scriptsLoaded.insert( Include );
				g_ScriptingHost.RunScript( Include );
			}

			CStr Inline = Child.getText();
			if( Inline.Length() )
			{
				g_ScriptingHost.RunMemScript( Inline.c_str(), Inline.Length(), filename.c_str(), Child.getLineNumber() );
			}
		}
		else if (ChildName == el_Traits)
		{
			XMBElementList TraitChildren = Child.getChildNodes();
			for(int j = 0; j < TraitChildren.Count; ++j)
			{
				XMBElement TraitChild = TraitChildren.item(j);
				int TraitChildName = TraitChild.getNodeName();
				if( TraitChildName == el_Footprint )
				{
					XMBElementList FootprintChildren = TraitChild.getChildNodes();
					float radius=0, height=0, width=0, depth=0;
					bool hadRadius = false, hadDepth = false;
					for(int k = 0; k < FootprintChildren.Count; ++k)
					{
						XMBElement FootprintChild = FootprintChildren.item(k);
						int FootprintChildName = FootprintChild.getNodeName();
						if( FootprintChildName == el_Radius )
						{
							hadRadius = true;
							radius = CStrW( FootprintChild.getText() ).ToFloat();
						}
						else if( FootprintChildName == el_Width )
						{
							width = CStrW( FootprintChild.getText() ).ToFloat();
						}
						else if( FootprintChildName == el_Height )
						{
							height = CStrW( FootprintChild.getText() ).ToFloat();
						}
						else if( FootprintChildName == el_Depth )
						{
							hadDepth = true;
							depth = CStrW( FootprintChild.getText() ).ToFloat();
						}
					}
					if( hadRadius ) 
					{
						// Specifying a circular footprint
						if( !m_bound_circle )
							m_bound_circle = new CBoundingCircle();
						m_bound_circle->setRadius( radius );
						m_bound_circle->setHeight( height );
						m_bound_type = CBoundingObject::BOUND_CIRCLE;
					}
					else if( hadDepth )
					{
						// Specifying a rectangular footprint
						if( !m_bound_box )
							m_bound_box = new CBoundingBox();
						m_bound_box->setDimensions( width, depth );
						m_bound_box->setHeight( height );
						m_bound_type = CBoundingObject::BOUND_OABB;
					}
					// Else, entity has no footprint.
				}
			}
			// important so that scripts can see traits
			XMLLoadProperty( XeroFile, Child, CStrW() );
		}
		else if( ChildName == el_Event )
		{
			// Action...On for consistency with the GUI.
			CStrW EventName = L"on" + (CStrW)Child.getAttributes().getNamedItem( at_On );

			CStrW Code (Child.getText());
			utf16string ExternalFunction = Child.getAttributes().getNamedItem( at_Function );

			// Does a property with this name already exist?

			for( uint eventID = 0; eventID < EVENT_LAST; eventID++ )
			{
				if( CStrW( EventNames[eventID] ) == EventName )
				{
					if( ExternalFunction != utf16string() )
					{
						jsval fnval;
						JSBool ret = JS_GetUCProperty( g_ScriptingHost.GetContext(), g_ScriptingHost.GetGlobalObject(), ExternalFunction.c_str(), ExternalFunction.size(), &fnval );
						debug_assert( ret );
						JSFunction* fn = JS_ValueToFunction( g_ScriptingHost.GetContext(), fnval );
						if( !fn )
						{
							LOG( ERROR, LOG_CATEGORY, "CEntityTemplate::LoadXML: Function does not exist for event %hs in file %s. Load failed.", EventName.c_str(), filename.c_str() );
							break;
						}
						m_EventHandlers[eventID].SetFunction( fn );
					}
					else
						m_EventHandlers[eventID].Compile( CStrW( filename ) + L"::" + EventName + L" (" + CStrW( Child.getLineNumber() ) + L")", Code );
					HasProperty( EventName )->m_Inherited = false;
					break;
				}
			}
		}
		else
		{
			XMLLoadProperty( XeroFile, Child, CStrW() );
		}
	}	


	if( m_player == 0 )
	{
		m_unmodified = this;
	}
	else
	{
		m_unmodified = g_EntityTemplateCollection.getTemplate( m_Tag, 0 );
	}

	return true;
}

/**
 * Converts the given string, consisting of underscores and letters,
 * to camelCase: the first letter of each underscore-separated "word"
 * is changed to lowercase. For example, the string MiniMap_Colour
 * is converted to miniMap_colour. This is consistent with the 
 * previous casing of entity attributes (all lowercase, with words
 * separated by underscores), while allowing us to switch over to
 * camelCase identifiers.
 *
 * @param const std::string & str The string to convert.
 * @return std::string The given string in camelCase.
 **/
std::string toCamelCase( const std::string& str )
{
	std::string ret = str;
	for( size_t i=0; i<ret.size(); i++ )
	{
		if( i==0 || ret[i-1] == '_' )
		{
			ret[i] = tolower( ret[i] );
		}
	}
	return ret;
}

void CEntityTemplate::XMLLoadProperty( const CXeromyces& XeroFile, const XMBElement& Source, const CStrW& BasePropertyName )
{
	// Add a property, put the node text into it.
	CStrW PropertyName = BasePropertyName + CStrW( toCamelCase( XeroFile.getElementString( Source.getNodeName() ) ) );

	IJSComplexProperty* Existing = HasProperty( PropertyName );
	if( Existing )
	{	
		if( !Existing->m_Intrinsic )
			LOG( WARNING, LOG_CATEGORY, "CEntityTemplate::XMLAddProperty: %ls already defined for %ls. Property trees will be merged.", PropertyName.c_str(), m_Tag.c_str() );
		Existing->Set( this, JSParseString( Source.getText() ) );
		//Existing->m_Inherited = false;
	}
	else
	{
		if( !Source.getText().length() )
		{
			// Arbitrarily say that if a node has no other value, define it to be 'true'.
			// Appears to be the most convenient thing to do in most circumstances.
			AddProperty( PropertyName, JSVAL_TRUE );
		}
		else
		{
			AddProperty( PropertyName, Source.getText() );
		}
	}

	
	PropertyName += CStrW( L"." );

	// Retrieve any attributes it has and add them as subproperties.
	XMBAttributeList AttributeSet = Source.getAttributes();
	for( unsigned int AttributeID = 0; AttributeID < (unsigned int)AttributeSet.Count; AttributeID++ )
	{
		XMBAttribute Attribute = AttributeSet.item( AttributeID );
		CStrW AttributeName = PropertyName + CStr8( toCamelCase( XeroFile.getAttributeString( Attribute.Name ) ) );
		Existing = HasProperty( AttributeName );
		
		if( Existing )
		{
			Existing->Set( this, JSParseString( Attribute.Value ) );
			Existing->m_Inherited = false;
		}
		else
			AddProperty( AttributeName, Attribute.Value );
	}

	// Retrieve any child nodes the property has and, similarly, add them as subproperties.
	XMBElementList NodeSet = Source.getChildNodes();
	for( unsigned int NodeID = 0; NodeID < (unsigned int)NodeSet.Count; NodeID++ )
	{
		XMBElement Node = NodeSet.item( NodeID );
		XMLLoadProperty( XeroFile, Node, PropertyName );
	}

}

/*
	Scripting Interface
*/

// Scripting initialization

void CEntityTemplate::ScriptingInit()
{
	AddMethod<jsval, &CEntityTemplate::ToString>( "toString", 0 );

	AddClassProperty( L"traits.id.classes", (GetFn)&CEntityTemplate::getClassSet, (SetFn)&CEntityTemplate::setClassSet );
	
	AddClassProperty( L"actions.move.speedCurr", &CEntityTemplate::m_speed );
	AddClassProperty( L"actions.move.turningRadius", &CEntityTemplate::m_turningRadius );
	AddClassProperty( L"actions.move.run.speed", &CEntityTemplate::m_runSpeed );
	AddClassProperty( L"actions.move.run.rangeMin", &CEntityTemplate::m_runMinRange );
	AddClassProperty( L"actions.move.run.range", &CEntityTemplate::m_runMaxRange );
	AddClassProperty( L"actions.move.run.regenRate", &CEntityTemplate::m_runRegenRate );
	AddClassProperty( L"actions.move.run.decayRate", &CEntityTemplate::m_runDecayRate );
	AddClassProperty( L"actions.move.passThroughAllies", &CEntityTemplate::m_passThroughAllies );
	AddClassProperty( L"actor", &CEntityTemplate::m_actorName );
    AddClassProperty( L"traits.health.max", &CEntityTemplate::m_healthMax );
    AddClassProperty( L"traits.health.barHeight", &CEntityTemplate::m_healthBarHeight );
	AddClassProperty( L"traits.health.barSize", &CEntityTemplate::m_healthBarSize );
	AddClassProperty( L"traits.health.barWidth", &CEntityTemplate::m_healthBarWidth );
	AddClassProperty( L"traits.health.borderHeight", &CEntityTemplate::m_healthBorderHeight);
	AddClassProperty( L"traits.health.borderWidth", &CEntityTemplate::m_healthBorderWidth );
	AddClassProperty( L"traits.health.borderName", &CEntityTemplate::m_healthBorderName );
	AddClassProperty( L"traits.health.regenRate", &CEntityTemplate::m_healthRegenRate );
	AddClassProperty( L"traits.health.regenStart", &CEntityTemplate::m_healthRegenStart );
	AddClassProperty( L"traits.health.decayRate", &CEntityTemplate::m_healthDecayRate );
    AddClassProperty( L"traits.stamina.max", &CEntityTemplate::m_staminaMax );
    AddClassProperty( L"traits.stamina.barHeight", &CEntityTemplate::m_staminaBarHeight );
	AddClassProperty( L"traits.stamina.barSize", &CEntityTemplate::m_staminaBarSize );
	AddClassProperty( L"traits.stamina.barWidth", &CEntityTemplate::m_staminaBarWidth );
	AddClassProperty( L"traits.stamina.borderHeight", &CEntityTemplate::m_staminaBorderHeight);
	AddClassProperty( L"traits.stamina.borderWidth", &CEntityTemplate::m_staminaBorderWidth );
	AddClassProperty( L"traits.stamina.borderName", &CEntityTemplate::m_staminaBorderName );
	AddClassProperty( L"traits.rally.name", &CEntityTemplate::m_rallyName );
	AddClassProperty( L"traits.rally.width", &CEntityTemplate::m_rallyWidth );
	AddClassProperty( L"traits.rally.height", &CEntityTemplate::m_rallyHeight );
	AddClassProperty( L"traits.flankPenalty.sectors", &CEntityTemplate::m_sectorDivs );
	AddClassProperty( L"traits.pitch.sectors", &CEntityTemplate::m_pitchDivs );
	AddClassProperty( L"traits.rank.width", &CEntityTemplate::m_rankWidth );
	AddClassProperty( L"traits.rank.height", &CEntityTemplate::m_rankHeight );
	AddClassProperty( L"traits.rank.name", &CEntityTemplate::m_rankName );
	AddClassProperty( L"traits.miniMap.type", &CEntityTemplate::m_minimapType );
	AddClassProperty( L"traits.miniMap.red", &CEntityTemplate::m_minimapR );
	AddClassProperty( L"traits.miniMap.green", &CEntityTemplate::m_minimapG );
	AddClassProperty( L"traits.miniMap.blue", &CEntityTemplate::m_minimapB );
	AddClassProperty( L"traits.anchor.type", &CEntityTemplate::m_anchorType );
	AddClassProperty( L"traits.anchor.conformX", &CEntityTemplate::m_anchorConformX );
	AddClassProperty( L"traits.anchor.conformZ", &CEntityTemplate::m_anchorConformZ );
	AddClassProperty( L"traits.vision.los", &CEntityTemplate::m_los );
	AddClassProperty( L"traits.vision.permanent", &CEntityTemplate::m_visionPermanent );
	AddClassProperty( L"traits.display.bars.enabled", &CEntityTemplate::m_barsEnabled );
	AddClassProperty( L"traits.display.bars.offset", &CEntityTemplate::m_barOffset );
	AddClassProperty( L"traits.display.bars.height", &CEntityTemplate::m_barHeight );
	AddClassProperty( L"traits.display.bars.width", &CEntityTemplate::m_barWidth );
	AddClassProperty( L"traits.display.bars.border", &CEntityTemplate::m_barBorder );
	AddClassProperty( L"traits.display.bars.borderSize", &CEntityTemplate::m_barBorderSize );
	AddClassProperty( L"traits.isTerritoryCentre", &CEntityTemplate::m_isTerritoryCentre );
	AddClassProperty( L"traits.creation.foundation", &CEntityTemplate::m_foundation );
	AddClassProperty( L"traits.creation.socket", &CEntityTemplate::m_socket );
	AddClassProperty( L"traits.creation.territoryRestriction", &CEntityTemplate::m_territoryRestriction );

	CJSComplex<CEntityTemplate>::ScriptingInit( "EntityTemplate" );
}

// Script-bound functions

JSObject* CEntityTemplate::GetScriptExecContext( IEventTarget* target )
{ 
	return( target->GetScriptExecContext( target ) );
}

jsval CEntityTemplate::ToString( JSContext* cx, uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	wchar_t buffer[256];
	if( m_player == 0 )
		swprintf( buffer, 256, L"[object EntityTemplate: %ls base]", m_Tag.c_str() );
	else
		swprintf( buffer, 256, L"[object EntityTemplate: %ls for player %d]", m_Tag.c_str(), m_player->GetPlayerID() );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}
