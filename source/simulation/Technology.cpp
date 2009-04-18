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
#include "Technology.h"
#include "TechnologyCollection.h"
#include "EntityManager.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "scripting/ScriptingHost.h"
#include "ps/XML/Xeromyces.h"
#include "ps/XML/XeroXMB.h"
#include "Entity.h"
#include "EntityTemplate.h"
#include "EntityTemplateCollection.h"
#include "ps/Player.h"
#include "scripting/ScriptableComplex.inl"

#define LOG_CATEGORY "Techs"

STL_HASH_SET<CStr, CStr_hash_compare> CTechnology::m_scriptsLoaded;

CTechnology::CTechnology( const CStrW& name, CPlayer* player )
: m_Name(name), m_player(player)
{
	m_researched = false;
	m_excluded = false;
	m_inProgress = false;
} 

bool CTechnology::LoadXml( const CStr& filename )
{
	CXeromyces XeroFile;

	if (XeroFile.Load(filename) != PSRETURN_OK)
        return false;

#define EL(x) int el_##x = XeroFile.GetElementID(#x)

	EL(tech);
	EL(id);
	EL(req);
	EL(effect);
	
#undef EL

	XMBElement Root = XeroFile.GetRoot();
	if ( Root.GetNodeName() != el_tech )
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CTechnology: XML root was not \"Tech\" in file %s. Load failed.", filename.c_str() );
		return false;
	}
	XMBElementList RootChildren = Root.GetChildNodes();
	bool ret;
	for  ( int i=0; i<RootChildren.Count; ++i )
	{
		XMBElement element = RootChildren.Item(i);
		int name = element.GetNodeName();
		if ( name == el_id )
			ret = LoadElId( element, XeroFile );
		else if ( name == el_req )
			ret = LoadElReq( element, XeroFile );
		else if ( name == el_effect )
			ret = LoadElEffect( element, XeroFile, filename );
		else 
			continue;
		if ( !ret )
		{
			LOG(CLogger::Error, LOG_CATEGORY, "CTechnology: Load failed for file %s", filename.c_str() );
			return false;
		}
	}
	
	return true;	
}

bool CTechnology::LoadElId( XMBElement ID, CXeromyces& XeroFile )
{
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	
	EL(generic);
	EL(specific);
	EL(icon);
	EL(icon_cell);
	EL(classes);
	EL(rollover);
	EL(history);

#undef EL

	XMBElementList children = ID.GetChildNodes();
	for ( int i=0; i<children.Count; ++i )
	{
		XMBElement element = children.Item(i);
		int name = element.GetNodeName();
		CStr8 value = element.GetText();
		
		if ( name == el_generic )
			m_Generic = value;
		else if ( name == el_specific )
			m_Specific = value;
		else if ( name == el_icon )
			m_Icon = value;
		else if ( name == el_icon_cell )
			m_IconCell = value.ToInt();
		else if ( name == el_classes )
			m_Classes = value;
		else if ( name == el_rollover )
			continue;	
		else if ( name == el_history )
			m_History = value;
		else
		{
			const char* tagName = XeroFile.GetElementString(name).c_str();
			LOG(CLogger::Error, LOG_CATEGORY, "CTechnology: invalid tag %s for XML file", tagName );
			return false;
		}
	}
	return true;
}

bool CTechnology::LoadElReq( XMBElement Req, CXeromyces& XeroFile )
{
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	
	EL(time);
	EL(resource);
	EL(tech);
	EL(entity);
	
#undef EL

	XMBElementList children = Req.GetChildNodes();
	for ( int i=0; i<children.Count; ++i )
	{
		XMBElement element = children.Item(i);
		int name = element.GetNodeName();
		CStr8 value = element.GetText();
		
		if ( name == el_time )
			m_ReqTime = value.ToFloat();
		else if ( name == el_resource )
		{
			XMBElementList resChildren = element.GetChildNodes();
			for ( int j=0; j<resChildren.Count; ++j )
			{
				XMBElement resElement = resChildren.Item(j);
				CStr8 resName = XeroFile.GetElementString( resElement.GetNodeName() );
				CStr8 resValue = resElement.GetText();

				// Add each resource as a property using its name in the XML file
				AddProperty( CStrW(resName).LowerCase(), resValue);
			}
		}
		else if ( name == el_entity )
		{
			m_ReqEntities.push_back( value );
		}
		else if ( name == el_tech )
		{
			m_ReqTechs.push_back( value );
		}
		else
		{
			const char* tagName = XeroFile.GetElementString(name).c_str();
			LOG(CLogger::Error, LOG_CATEGORY, "CTechnology: invalid tag %s for XML file", tagName );
			return false;
		}
	}
	return true;
}

bool CTechnology::LoadElEffect( XMBElement effect, CXeromyces& XeroFile, const CStr& filename )
{
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)

	EL(target);
	EL(pair);
	EL(modifier);
	EL(attribute);
	EL(value);
	EL(set);
	EL(script);
	EL(function);
	AT(name);
	AT(file);
	
#undef EL
#undef AT

	XMBElementList children = effect.GetChildNodes();
	for ( int i=0; i<children.Count; ++i )
	{
		XMBElement element = children.Item(i);
		int name = element.GetNodeName();
		CStr value = element.GetText();

		if ( name == el_target )
			m_Targets.push_back(value);
		else if ( name == el_pair )
			m_Pairs.push_back(value);
		else if ( name == el_modifier )
		{
			XMBElementList modChildren = element.GetChildNodes();
			m_Modifiers.push_back(Modifier());
			for ( int j=0; j<modChildren.Count; ++j )
			{
				XMBElement modElement = modChildren.Item(j);
				CStrW modValue = modElement.GetText();

				if ( modElement.GetNodeName() == el_attribute)
					m_Modifiers.back().attribute = modValue;
				else if ( modElement.GetNodeName() == el_value )
				{
					if( modValue.size() == 0)
					{
						LOG(CLogger::Error, LOG_CATEGORY, "CTechnology::LoadXml: invalid Modifier value (empty string)" );
						m_Modifiers.pop_back();
						return false;
					}

					if( modValue[modValue.size()-1] == '%' )
					{
						m_Modifiers.back().isPercent = true;
						modValue = modValue.substr( 0, modValue.size()-1 );
					}
					m_Modifiers.back().value = modValue.ToFloat();
				}
				else
				{
					LOG(CLogger::Error, LOG_CATEGORY, "CTechnology::LoadXml: invalid tag inside \"Modifier\" tag" );
					m_Modifiers.pop_back();
					return false;
				}
			}
		}
		else if ( name == el_set )
		{
			XMBElementList setChildren = element.GetChildNodes();
			m_Sets.push_back(Modifier());
			for ( int j=0; j<setChildren.Count; ++j )
			{
				XMBElement setElement = setChildren.Item(j);
				CStrW setValue = setElement.GetText();

				if ( setElement.GetNodeName() == el_attribute)
					m_Sets.back().attribute = setValue;
				else if ( setElement.GetNodeName() == el_value )
					m_Sets.back().value = setValue.ToFloat();
				else
				{
					LOG(CLogger::Error, LOG_CATEGORY, "CTechnology::LoadXml: invalid tag inside \"Set\" tag" );
					m_Sets.pop_back();
					return false;
				}
			}
		}
		else if ( name == el_script )
		{
			CStr Include = element.GetAttributes().GetNamedItem( at_file );
			if( !Include.empty() && m_scriptsLoaded.find( Include ) == m_scriptsLoaded.end() )
			{
				m_scriptsLoaded.insert( Include );
				g_ScriptingHost.RunScript( Include );
			}
			CStr Inline = element.GetText();
			if( !Inline.empty() )
			{
				g_ScriptingHost.RunMemScript( Inline.c_str(), Inline.length(), filename, element.GetLineNumber() );
			}
		}
		else if ( name == el_function )
		{
			utf16string funcName = element.GetAttributes().GetNamedItem( at_name );
			CStr Inline = element.GetText();

			if ( funcName != utf16string() )
			{
				jsval fnval;
				JSBool ret = JS_GetUCProperty( g_ScriptingHost.GetContext(), g_ScriptingHost.GetGlobalObject(), funcName.c_str(), funcName.size(), &fnval );
				debug_assert( ret );
				JSFunction* fn = JS_ValueToFunction( g_ScriptingHost.GetContext(), fnval );
				if( !fn )
				{
					LOG(CLogger::Error, LOG_CATEGORY, "CTechnology::LoadXml: Function does not exist for %hs in file %s. Load failed.", funcName.c_str(), filename.c_str() );
					return false;
				}
				m_effectFunction.SetFunction( fn );
			}
			else if ( Inline != CStr() )
				m_effectFunction.Compile( CStrW( filename ) + L"::" + (CStrW)funcName + L" (" + CStrW( element.GetLineNumber() ) + L")", Inline );
			//(No error needed; scripts are optional)
		}
		else
		{
			const char* tagName = XeroFile.GetElementString(name).c_str();
			LOG(CLogger::Error, LOG_CATEGORY, "CTechnology: invalid tag %s for XML file", tagName );
			return false;
		}
	}
	return true;
}

bool CTechnology::IsTechValid()
{
	if ( m_excluded || m_inProgress )
		return false;

	for( size_t i=0; i<m_Pairs.size(); i++ )
	{
		if( g_TechnologyCollection.GetTechnology( m_Pairs[i], m_player )->m_inProgress )
			return false;
	}

	return ( HasReqEntities() && HasReqTechs() );
}

bool CTechnology::HasReqEntities()
{
	// Check whether we have ALL the required entities.

	std::vector<HEntity> entities;
	m_player->GetControlledEntities(entities);
	for ( std::vector<CStr>::iterator it=m_ReqEntities.begin(); it != m_ReqEntities.end(); it++ )
	{
		// For each required class, check that we have it
		bool got = false;
		for( CEntityList::iterator it2=entities.begin(); it2 != entities.end(); it2++ )
		{
			if ( (*it2)->m_classes.IsMember(*it) )
			{	
				got = true;
				break;
			}
		}
		if( !got )
			return false;
	}
	return true;
}

bool CTechnology::HasReqTechs()
{
	// Check whether we have ANY of the required techs (this is slightly confusing but required for 
	// the way the tech system is currently planned; ideally we'd have an <Or> or <And> in the XML).

	if ( m_ReqTechs.size() == 0 )
		return true;

	for ( std::vector<CStr>::iterator it=m_ReqTechs.begin(); it != m_ReqTechs.end(); it++ )
	{
		if ( g_TechnologyCollection.GetTechnology( (CStrW)*it, m_player )->IsResearched() )
		{	
			return true;
		}
	}
	return false;
}

void CTechnology::Apply( CEntity* entity )
{
	// Find out if the unit has one of our target classes
	bool ok = false;
	for ( std::vector<CStr>::iterator it = m_Targets.begin(); it != m_Targets.end(); it++ )
	{
		if ( entity->m_classes.IsMember( *it ) )
		{
			ok = true;
			break;
		}
	}
	if( !ok ) return;

	// Apply modifiers
	for ( std::vector<Modifier>::iterator mod=m_Modifiers.begin(); mod!=m_Modifiers.end(); mod++ )
	{
		jsval oldVal;
		if( entity->GetProperty( g_ScriptingHost.getContext(), mod->attribute, &oldVal ) )
		{
			float modValue;
			if( mod->isPercent )
			{
				jsval baseVal;
				entity->m_base->m_unmodified->GetProperty( g_ScriptingHost.getContext(), mod->attribute, &baseVal );
				modValue = ToPrimitive<float>(baseVal) * mod->value / 100.0f;
			}
			else
			{
				modValue = mod->value;
			}

			jsval newVal = ToJSVal( ToPrimitive<float>(oldVal) + modValue );
			entity->SetProperty( g_ScriptingHost.GetContext(), mod->attribute, &newVal );
		}
	}

	// Apply sets
	for ( std::vector<Modifier>::iterator mod=m_Sets.begin(); mod!=m_Sets.end(); mod++ )
	{
		jsval newVal = ToJSVal( mod->value );
		entity->SetProperty( g_ScriptingHost.GetContext(), mod->attribute, &newVal );
	}
}


//JS stuff

void CTechnology::ScriptingInit()
{
	AddClassProperty(L"name", &CTechnology::m_Name, true);
	AddClassProperty(L"player", &CTechnology::m_player, true);
	AddClassProperty(L"generic", &CTechnology::m_Generic, true);
	AddClassProperty(L"specific", &CTechnology::m_Specific, true);
	AddClassProperty(L"icon", &CTechnology::m_Icon);	//GUI might want to change this...?
	AddClassProperty(L"icon_cell", &CTechnology::m_IconCell);
	AddClassProperty(L"classes", &CTechnology::m_Classes, true);
	AddClassProperty(L"history", &CTechnology::m_History, true);

	AddClassProperty(L"time", &CTechnology::m_ReqTime);	//Techs may upgrade research time and cost of other techs
	AddClassProperty(L"in_progress", &CTechnology::m_inProgress);

	AddMethod<bool, &CTechnology::ApplyEffects>( "applyEffects", 2 );
	AddMethod<bool, &CTechnology::IsExcluded>( "isExcluded", 0 );
	AddMethod<bool, &CTechnology::IsValid>( "isValid", 0 );
	AddMethod<bool, &CTechnology::IsResearched>( "isResearched", 0 );
	AddMethod<size_t, &CTechnology::GetPlayerID>( "getPlayerID", 0 );

	CJSComplex<CTechnology>::ScriptingInit("Technology");
}

bool CTechnology::ApplyEffects( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	// Unmark ourselves as in progress
	m_inProgress = false;

	if ( !IsTechValid() )
	{
		return false;
	}

	// Disable any paired techs
	for ( std::vector<CStr>::iterator it=m_Pairs.begin(); it != m_Pairs.end(); it++ )
		g_TechnologyCollection.GetTechnology(*it, m_player)->SetExclusion(true);

	// Disable ourselves so we can't be researched twice
	m_excluded = true;

	// Mark ourselves as researched
	m_researched = true;

	// Apply effects to all entities
	std::vector<HEntity> entities;
	m_player->GetControlledEntities(entities);
	for ( size_t i=0; i<entities.size(); ++i )
	{
		Apply( entities[i] );
	}
	
	// Run one-time tech script
	if( m_effectFunction )
	{
		jsval rval;
		jsval arg = ToJSVal( m_player );
		m_effectFunction.Run( this->GetScript(), &rval, 1, &arg );
	}

	// Add ourselves to player's researched techs
	m_player->AddActiveTech( this );

	return true;
}

bool CTechnology::IsValid( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return IsTechValid();
}

bool CTechnology::IsExcluded( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return m_excluded;
}

bool CTechnology::IsResearched( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return IsResearched();
}

size_t CTechnology::GetPlayerID( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return m_player->GetPlayerID();
}


