/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "EntityTemplate.h"
#include "EntityTemplateCollection.h"
#include "graphics/ObjectManager.h"
#include "ps/CStr.h"
#include "ps/Player.h"
#include "scripting/ScriptableComplex.inl"
#include "ps/XML/Xeromyces.h"
#include "sound/SoundGroupMgr.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "entity"

STL_HASH_SET<CStr, CStr_hash_compare> CEntityTemplate::scriptsLoaded;

// Utility functions used in reading XML
namespace {
	/**
	 * Converts the given string, consisting of underscores and letters,
	 * to camelCase: the first letter of each underscore-separated "word"
	 * is changed to lowercase. For example, the string MiniMap_Colour
	 * is converted to miniMap_colour.
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
}

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

void CEntityTemplate::LoadBase()
{ 
	// Copy the parent's bounds, unless we're providing a replacement.
	if( m_bound_type == CBoundingObject::BOUND_NONE )
	{
		if( m_base->m_bound_type == CBoundingObject::BOUND_CIRCLE )
		{
 			m_bound_circle = new CBoundingCircle();
			m_bound_circle->SetRadius( m_base->m_bound_circle->m_radius );
			m_bound_circle->SetHeight( m_base->m_bound_circle->m_height );
		}
		else if( m_base->m_bound_type == CBoundingObject::BOUND_OABB )
		{
			m_bound_box = new CBoundingBox();
			m_bound_box->SetDimensions( m_base->m_bound_box->GetWidth(), m_base->m_bound_box->GetDepth() );
			m_bound_box->SetHeight( m_base->m_bound_box->m_height );
		}
		m_bound_type = m_base->m_bound_type;
	}

	// Initialize sound group table from the parent's sound group table
	for(SoundGroupTable::iterator it = m_base->m_SoundGroupTable.begin(); 
			it != m_base->m_SoundGroupTable.end(); ++it) 
	{
		m_SoundGroupTable[it->first] = it->second;
	}

	SetBase( m_base );
	m_classes.SetParent( &( m_base->m_classes ) );
	SetNextObject( m_base );
}

jsval CEntityTemplate::GetClassSet()
{
	CStrW result = m_classes.GetMemberList(); 
	return( ToJSVal( result ) );
}

void CEntityTemplate::SetClassSet( jsval value )
{
	CStr memberCmdList = ToPrimitive<CStrW>( value );
	m_classes.SetFromMemberList(memberCmdList);

	RebuildClassSet();
}

void CEntityTemplate::RebuildClassSet()
{
	m_classes.Rebuild();
	InheritorsList::iterator it;
	for( it = m_Inheritors.begin(); it != m_Inheritors.end(); it++ )
		(*it)->RebuildClassSet();
}

bool CEntityTemplate::LoadXml( const CStr& filename )
{
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		// Fail
		return false;

	// Define all the elements and attributes used in the XML file
	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
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
	EL(SoundGroups);
	AT(Parent);
	AT(On);
	AT(File);
	AT(Function);
	#undef AT
	#undef EL

	XMBElement Root = XeroFile.GetRoot();

	if( Root.GetNodeName() != el_Entity )
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CEntityTemplate::LoadXml: XML root was not \"Entity\" in file %s. Load failed.", filename.c_str() );
		return( false );
	}

	XMBElementList RootChildren = Root.GetChildNodes();

	m_Tag = CStr(filename).AfterLast("/").BeforeLast(".xml");

	m_Base_Name = Root.GetAttributes().GetNamedItem( at_Parent );

	// Load our parent, if we have one
	if( ! m_Base_Name.empty() )
	{
		CEntityTemplate* base = g_EntityTemplateCollection.GetTemplate( m_Base_Name, m_player );
		if( base )
		{
			m_base = base;
			LoadBase();
		}
		else
		{
			LOG(CLogger::Warning, LOG_CATEGORY, "Parent template \"%ls\" does not exist in template \"%ls\"", m_Base_Name.c_str(), m_Tag.c_str() );
			// (The requested entity will still be returned, but with no parent.
			// Is this a reasonable thing to do?)
		}
	}
	
	for (int i = 0; i < RootChildren.Count; ++i)
	{
		XMBElement Child = RootChildren.Item(i);

		int ChildName = Child.GetNodeName();
		if( ChildName == el_Script )
		{
			CStr Include = Child.GetAttributes().GetNamedItem( at_File );

			if( !Include.empty() && scriptsLoaded.find( Include ) == scriptsLoaded.end() )
			{
				scriptsLoaded.insert( Include );
				g_ScriptingHost.RunScript( Include );
			}

			CStr Inline = Child.GetText();
			if( !Inline.empty() )
			{
				g_ScriptingHost.RunMemScript( Inline.c_str(), Inline.length(), filename.c_str(), Child.GetLineNumber() );
			}
		}
		else if (ChildName == el_Traits)
		{
			XMBElementList TraitChildren = Child.GetChildNodes();
			for(int j = 0; j < TraitChildren.Count; ++j)
			{
				XMBElement TraitChild = TraitChildren.Item(j);
				int TraitChildName = TraitChild.GetNodeName();
				if( TraitChildName == el_Footprint )
				{
					XMBElementList FootprintChildren = TraitChild.GetChildNodes();
					float radius=0, height=0, width=0, depth=0;
					bool hadRadius = false, hadDepth = false;
					for(int k = 0; k < FootprintChildren.Count; ++k)
					{
						XMBElement FootprintChild = FootprintChildren.Item(k);
						int FootprintChildName = FootprintChild.GetNodeName();
						if( FootprintChildName == el_Radius )
						{
							hadRadius = true;
							radius = CStrW( FootprintChild.GetText() ).ToFloat();
						}
						else if( FootprintChildName == el_Width )
						{
							width = CStrW( FootprintChild.GetText() ).ToFloat();
						}
						else if( FootprintChildName == el_Height )
						{
							height = CStrW( FootprintChild.GetText() ).ToFloat();
						}
						else if( FootprintChildName == el_Depth )
						{
							hadDepth = true;
							depth = CStrW( FootprintChild.GetText() ).ToFloat();
						}
					}
					if( hadRadius ) 
					{
						// Specifying a circular footprint
						if( !m_bound_circle )
							m_bound_circle = new CBoundingCircle();
						m_bound_circle->SetRadius( radius );
						m_bound_circle->SetHeight( height );
						m_bound_type = CBoundingObject::BOUND_CIRCLE;
					}
					else if( hadDepth )
					{
						// Specifying a rectangular footprint
						if( !m_bound_box )
							m_bound_box = new CBoundingBox();
						m_bound_box->SetDimensions( width, depth );
						m_bound_box->SetHeight( height );
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
			CStrW EventName = L"on" + (CStrW)Child.GetAttributes().GetNamedItem( at_On );

			CStrW Code (Child.GetText());
			utf16string ExternalFunction = Child.GetAttributes().GetNamedItem( at_Function );

			// Does a property with this name already exist?

			for( size_t eventID = 0; eventID < EVENT_LAST; eventID++ )
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
							LOG(CLogger::Error, LOG_CATEGORY, "CEntityTemplate::LoadXml: Function does not exist for event %ls in file %s. Load failed.", EventName.c_str(), filename.c_str() );
							break;
						}
						m_EventHandlers[eventID].SetFunction( fn );
					}
					else
						m_EventHandlers[eventID].Compile( CStrW( filename ) + L"::" + EventName + L" (" + CStrW( Child.GetLineNumber() ) + L")", Code );
					HasProperty( EventName )->m_Inherited = false;
					break;
				}
			}
		}
		else if( ChildName == el_SoundGroups )
		{
			// Read every child element's value into m_SoundGroupTable with its tag as the key
			XMBElementList children = Child.GetChildNodes();
			for(int j = 0; j < children.Count; ++j)
			{
				XMBElement child = children.Item(j);
				CStr8 name = toCamelCase( XeroFile.GetElementString( child.GetNodeName() ) );
				CStr8 soundGroupFilename = child.GetText();
				const size_t soundGroupIndex = g_soundGroupMgr->AddGroup(soundGroupFilename);
				m_SoundGroupTable[name] = soundGroupIndex;
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
		m_unmodified = g_EntityTemplateCollection.GetTemplate( m_Tag, 0 );
	}

	return true;
}

void CEntityTemplate::XMLLoadProperty( const CXeromyces& XeroFile, const XMBElement& Source, const CStrW& BasePropertyName )
{
	// Add a property, put the node text into it.
	CStrW PropertyName = BasePropertyName + CStrW( toCamelCase( XeroFile.GetElementString( Source.GetNodeName() ) ) );

	IJSComplexProperty* Existing = HasProperty( PropertyName );
	if( Existing )
	{	
		if( !Existing->m_Intrinsic )
			LOG(CLogger::Warning, LOG_CATEGORY, "CEntityTemplate::XMLAddProperty: %ls already defined for %ls. Property trees will be merged.", PropertyName.c_str(), m_Tag.c_str() );
		Existing->Set( this, JSParseString( Source.GetText() ) );
		//Existing->m_Inherited = false;
	}
	else
	{
		if( !Source.GetText().length() )
		{
			// Arbitrarily say that if a node has no other value, define it to be 'true'.
			// Appears to be the most convenient thing to do in most circumstances.
			AddProperty( PropertyName, JSVAL_TRUE );
		}
		else
		{
			AddProperty( PropertyName, Source.GetText() );
		}
	}

	
	PropertyName += CStrW( L"." );

	// Retrieve any attributes it has and add them as subproperties.
	XMBAttributeList AttributeSet = Source.GetAttributes();
	for( int AttributeID = 0; AttributeID < AttributeSet.Count; AttributeID++ )
	{
		XMBAttribute Attribute = AttributeSet.Item( AttributeID );
		CStrW AttributeName = PropertyName + CStr8( toCamelCase( XeroFile.GetAttributeString( Attribute.Name ) ) );
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
	XMBElementList NodeSet = Source.GetChildNodes();
	for( int NodeID = 0; NodeID < NodeSet.Count; NodeID++ )
	{
		XMBElement Node = NodeSet.Item( NodeID );
		XMLLoadProperty( XeroFile, Node, PropertyName );
	}

}

/*
	Scripting Interface
*/

// Scripting initialization

void CEntityTemplate::ScriptingInit()
{
	AddMethod<CStr, &CEntityTemplate::ToString>( "toString", 0 );

	AddClassProperty( L"traits.id.classes", static_cast<GetFn>(&CEntityTemplate::GetClassSet), static_cast<SetFn>(&CEntityTemplate::SetClassSet) );
	
	AddClassProperty( L"actions.move.speed", &CEntityTemplate::m_speed );
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
	AddClassProperty( L"traits.pitch.value", &CEntityTemplate::m_pitchValue );
	AddClassProperty( L"traits.rank.width", &CEntityTemplate::m_rankWidth );
	AddClassProperty( L"traits.rank.height", &CEntityTemplate::m_rankHeight );
	AddClassProperty( L"traits.rank.name", &CEntityTemplate::m_rankName );
	AddClassProperty( L"traits.ai.stance.curr", &CEntityTemplate::m_stanceName );
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
	AddClassProperty( L"traits.creation.buildingLimitCategory", &CEntityTemplate::m_buildingLimitCategory );



	CJSComplex<CEntityTemplate>::ScriptingInit( "EntityTemplate" );
}

// Script-bound functions

JSObject* CEntityTemplate::GetScriptExecContext( IEventTarget* target )
{ 
	return( target->GetScriptExecContext( target ) );
}

CStr CEntityTemplate::ToString( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	if( m_player == 0 )
		return "[object EntityTemplate: " + CStr(m_Tag) + " base]";
	else
		return "[object EntityTemplate: " + CStr(m_Tag) + " for player " + CStr((unsigned)m_player->GetPlayerID()) + "]";
}
