#include "precompiled.h"
#include "Technology.h"
#include "TechnologyCollection.h"
#include "EntityManager.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "scripting/ScriptingHost.h"
#include "ps/XML/Xeromyces.h"
#include "ps/XML/XeroXMB.h"
#include "BaseEntity.h"
#include "Entity.h"
#include "ps/Player.h"

#define LOG_CATEGORY "Techs"

STL_HASH_SET<CStr, CStr_hash_compare> CTechnology::m_scriptsLoaded;

CTechnology::CTechnology() 
{
	ONCE( ScriptingInit(); );

	m_researched = m_excluded=false;
	m_effectFunction = NULL;
	m_JSFirst = false;
} 
bool CTechnology::loadXML( CStr filename )
{
	CXeromyces XeroFile;

	if (XeroFile.Load(filename) != PSRETURN_OK)
        return false;

	#define EL(x) int el_##x = XeroFile.getElementID(#x)

	EL(tech);
	EL(id);
	EL(req);
	EL(effect);
	
	#undef EL

	XMBElement Root = XeroFile.getRoot();
	if ( Root.getNodeName() != el_tech )
	{
		LOG( ERROR, LOG_CATEGORY, "CTechnology: XML root was not \"Tech\" in file %s. Load failed.", filename.c_str() );
		return false;
	}
	XMBElementList RootChildren = Root.getChildNodes();
	bool ret;
	for  ( int i=0; i<RootChildren.Count; ++i )
	{
		XMBElement element = RootChildren.item(i);
		int name = element.getNodeName();
		if ( name == el_id )
			ret = loadELID( element, XeroFile );
		else if ( name == el_req )
			ret = loadELReq( element, XeroFile );
		else if ( name == el_effect )
			ret = loadELEffect( element, XeroFile, filename );
		else 
			continue;
		if ( !ret )
		{
			LOG( ERROR, LOG_CATEGORY, "CTechnology: Load failed for file %s", filename.c_str() );
			return false;
		}
	}
	
	return true;	
}
bool CTechnology::loadELID( XMBElement ID, CXeromyces& XeroFile )
{
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	
	EL(generic);
	EL(specific);
	EL(icon);
	EL(icon_cell);
	EL(classes);
	EL(rollover);
	EL(history);

	#undef EL

	XMBElementList children = ID.getChildNodes();
	for ( int i=0; i<children.Count; ++i )
	{
		XMBElement element = children.item(i);
		int name = element.getNodeName();
		CStr value = CStr(element.getText());
		
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
			const char* tagName = XeroFile.getElementString(name).c_str();
			LOG( ERROR, LOG_CATEGORY, "CTechnology: invalid tag %s for XML file", tagName );
			return false;
		}
	}
	return true;
}
bool CTechnology::loadELReq( XMBElement Req, CXeromyces& XeroFile )
{
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	
	EL(time);
	EL(resource);
	EL(food);
	EL(tech);
	EL(stone);
	EL(ore);
	EL(wood);
	EL(entity);
	
	#undef EL

	XMBElementList children = Req.getChildNodes();
	for ( int i=0; i<children.Count; ++i )
	{
		XMBElement element = children.item(i);
		int name = element.getNodeName();
		CStr value = element.getText();
		
		if ( name == el_time )
			m_ReqTime = value.ToFloat();
		else if ( name == el_resource )
		{
			XMBElementList resChildren = element.getChildNodes();
			for ( int j=0; j<resChildren.Count; ++j )
			{
				XMBElement resElement = resChildren.item(j);
				int resName = resElement.getNodeName();
				CStr resValue = CStr(resElement.getText());

				if ( resName == el_food )	//NOT LOADED-GET CHILD NODES
					m_ReqFood = resValue.ToFloat();
				else if ( resName == el_wood )
					m_ReqWood = resValue.ToFloat();
				else if ( resName == el_stone )
					m_ReqStone = resValue.ToFloat();
				else if ( resName == el_ore )
					m_ReqOre = resValue.ToFloat();
				else
				{
					const char* tagName = XeroFile.getElementString(name).c_str();
					LOG( ERROR, LOG_CATEGORY, "CTechnology: invalid tag %s for XML file", tagName );
					return false;
				}
			}
		}
		else if ( name == el_entity )
			m_ReqEntities.push_back( value );
		else if ( name == el_tech )
			m_ReqTechs.push_back( value );
		else
		{
			const char* tagName = XeroFile.getElementString(name).c_str();
			LOG( ERROR, LOG_CATEGORY, "CTechnology: invalid tag %s for XML file", tagName );
			return false;
		}
	}
	return true;
}
bool CTechnology::loadELEffect( XMBElement effect, CXeromyces& XeroFile, CStr& filename )
{
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	#define AT(x) int at_##x = XeroFile.getAttributeID(#x)

	EL(target);
	EL(pair);
	EL(modifier);
	EL(attribute);
	EL(value);
	EL(set);
	EL(script);
	EL(function);
	AT(name);
	AT(order);
	AT(file);
	
	#undef EL
	#undef AT

	XMBElementList children = effect.getChildNodes();
	for ( int i=0; i<children.Count; ++i )
	{
		XMBElement element = children.item(i);
		int name = element.getNodeName();
		CStr value = element.getText();

		if ( name == el_target )
			m_Targets.push_back(value);
		else if ( name == el_pair )
			m_Pairs.push_back(value);
		
		else if ( name == el_modifier )
		{
			XMBElementList modChildren = element.getChildNodes();
			m_Modifiers.push_back(Modifier());
			for ( int j=0; j<modChildren.Count; ++j )
			{
				XMBElement modElement = modChildren.item(j);
				CStr modValue = CStr(modElement.getText());

				if ( modElement.getNodeName() == el_attribute)
				{
					if ( CEntity::m_AttributeTable.find( modValue ) == CEntity::m_AttributeTable.end() )
					{
						LOG( ERROR, LOG_CATEGORY, "CTechnology::loadXML invalid attribute %s for modifier attribute", modValue);
						m_Modifiers.pop_back();
						return false;
					}
					else
						m_Modifiers.back().attribute = modValue;
				}
				else if ( modElement.getNodeName() == el_value )
					m_Modifiers.back().value = modValue.ToFloat();
				else
				{
					LOG( ERROR, LOG_CATEGORY, "CTechnology::loadXML invalid tag inside \"Modifier\" tag" );
					m_Modifiers.pop_back();
					return false;
				}
			}
		}

		else if ( name == el_set )
		{
			XMBElementList setChildren = element.getChildNodes();
			m_Sets.push_back(Modifier());
			for ( int j=0; j<setChildren.Count; ++j )
			{
				XMBElement setElement = setChildren.item(j);
				CStr setValue = CStr(setElement.getText());

				if ( setElement.getNodeName() == el_attribute)
				{
					if ( CEntity::m_AttributeTable.find( setValue ) == CEntity::m_AttributeTable.end() )
					{
						LOG( ERROR, LOG_CATEGORY, "CTechnology::loadXML invalid attribute %s for \"set\" attribute", setValue);
						m_Sets.pop_back();
						return false;
					}
					else
						m_Sets.back().attribute = setValue;
				}
				else if ( setElement.getNodeName() == el_value )
					m_Sets.back().value = setValue.ToFloat();
				else
				{
					LOG( ERROR, LOG_CATEGORY, "CTechnology::loadXML invalid tag inside \"Set\" tag" );
					m_Sets.pop_back();
					return false;
				}
			}
		}
		
		else if ( name == el_script )
		{
			CStr Include = element.getAttributes().getNamedItem( at_file );
			if( Include.Length() && m_scriptsLoaded.find( Include ) == m_scriptsLoaded.end() )
			{
				m_scriptsLoaded.insert( Include );
				g_ScriptingHost.RunScript( Include );
			}
			CStr Inline = element.getText();
			if( Inline.Length() )
			{
				g_ScriptingHost.RunMemScript( Inline.c_str(), Inline.Length(), filename, element.getLineNumber() );
			}
		}
		else if ( name == el_function )
		{
			utf16string funcName = element.getAttributes().getNamedItem( at_name );
			CStr Inline = element.getText();
			//default to first
			m_JSFirst = (utf16string() == element.getAttributes().getNamedItem( at_order ));

			if ( funcName != utf16string() )
			{
				jsval fnval;
				JSBool ret = JS_GetUCProperty( g_ScriptingHost.GetContext(), g_ScriptingHost.GetGlobalObject(), funcName.c_str(), funcName.size(), &fnval );
				debug_assert( ret );
				JSFunction* fn = JS_ValueToFunction( g_ScriptingHost.GetContext(), fnval );
				if( !fn )
				{
					LOG( ERROR, LOG_CATEGORY, "CTechnology::LoadXML: Function does not exist for %hs in file %s. Load failed.", funcName.c_str(), filename.c_str() );
					return false;
				}
				m_effectFunction->SetFunction( fn );
			}
			else if ( Inline != CStr() )
				m_effectFunction->Compile( CStrW( filename ) + L"::" + (CStrW)funcName + L" (" + CStrW( element.getLineNumber() ) + L")", Inline );
			//(No error needed; scripts are optional)
		}
		else
		{
			const char* tagName = XeroFile.getElementString(name).c_str();
			LOG( ERROR, LOG_CATEGORY, "CTechnology: invalid tag %s for XML file", tagName );
			return false;
		}
	}
	return true;
}
bool CTechnology::isTechValid()
{
	if ( m_excluded )
		return false;
	if ( hasReqEntities() && hasReqTechs() )
		return true;
	return false;
}
bool CTechnology::hasReqEntities()
{
	bool ret=true;
	std::vector<HEntity>* entities = m_player->GetControlledEntities();
	for ( std::vector<CStr>::iterator it=m_ReqEntities.begin(); it != m_ReqEntities.end(); it++ )
	{
		for( CEntityList::iterator it2=entities->begin(); it2 != entities->end(); it2++ )
		{
			if ( (*it2)->m_classes.IsMember(*it) )
			{	
				ret=true;
				break;
			}
		}
	}
	delete entities;
	return ret;
}
bool CTechnology::hasReqTechs()
{
	bool ret=true;
	for ( std::vector<CStr>::iterator it=m_ReqTechs.begin(); it != m_ReqTechs.end(); it++ )
	{
		if ( !g_TechnologyCollection.getTechnology( (CStrW)*it )->isResearched() )
		{	
			ret=false;
			break;
		}
	}
	return ret;
}
//JS stuff

void CTechnology::ScriptingInit()
{
	AddProperty(L"generic", &CTechnology::m_Generic, true);
	AddProperty(L"specific", &CTechnology::m_Specific, true);
	AddProperty(L"icon", &CTechnology::m_Icon);	//GUI might want to change this...?
	AddProperty<int>(L"icon_cell", &CTechnology::m_IconCell);
	AddProperty(L"classes", &CTechnology::m_Classes, true);
	AddProperty(L"history", &CTechnology::m_History, true);

	AddProperty<float>(L"time", &CTechnology::m_ReqTime);	//Techs may upgrade research time and cost of other techs
	AddProperty<float>(L"food", &CTechnology::m_ReqFood);
	AddProperty<float>(L"wood", &CTechnology::m_ReqWood);
	AddProperty<float>(L"stone", &CTechnology::m_ReqStone);
	AddProperty<float>(L"ore", &CTechnology::m_ReqOre);

	AddMethod<jsval, &CTechnology::ApplyEffects>( "applyEffects", 1 );
	AddMethod<jsval, &CTechnology::IsExcluded>( "isExcluded", 0 );
	AddMethod<jsval, &CTechnology::IsValid>( "isValid", 0 );
	AddMethod<jsval, &CTechnology::IsResearched>( "isResearched", 0 );
	AddMethod<jsval, &CTechnology::GetPlayerID>( "getPlayerID", 0 );
	AddMethod<jsval, &CTechnology::IsJSFirst>( "isJSFirst", 0 );

	CJSObject<CTechnology>::ScriptingInit("Technology");

	debug_printf("CTechnology::ScriptingInit complete");
}

jsval CTechnology::ApplyEffects( JSContext* cx, uintN argc, jsval* argv )
{
	if ( !isTechValid() )
		return JS_FALSE;
	else if ( argc < 3 )
	{
		JS_ReportError(cx, "too few parameters for CTechnology::ApplyEffects.");
		return JS_FALSE;
	}

	m_player = g_Game->GetPlayer( ToPrimitive<PS_uint>( argv[0] ) );
	if( m_player == 0 ) 
	{
		JS_ReportError(cx, "invalid player number for CTechnology::ApplyEffects.");
		return JS_FALSE;
	}

	bool first = ToPrimitive<bool>( argv[1] );
	bool invert = ToPrimitive<bool>( argv[2] );

	//Optional type overriding if some in some special case the script wants to modify non-floats
	CStr varType("float");
	if ( argc == 4 )
		varType = ToPrimitive<CStr>( argv[3] );

	if ( first &&  m_effectFunction )
	{
		m_effectFunction->Run( this->GetScript() );
	}

	//Disable other templates
	for ( std::vector<CStr>::iterator it=m_Pairs.begin(); it != m_Pairs.end(); it++ )
		g_TechnologyCollection.getTechnology(*it)->setExclusion(true);
	
	std::vector<HEntity>* entities = m_player->GetControlledEntities();
	if ( entities->empty() )
	{
		delete entities;
		return JS_FALSE;
	}

	std::vector<HEntity> entitiesAffected;

	//Find which entities should be affected
	for ( size_t i=0; i<entities->size(); ++i )
	{
		for ( std::vector<CStr>::iterator it = m_Targets.begin(); it != m_Targets.end(); it++ )
		{
			if ( (*entities)[i]->m_classes.IsMember( *it ) )
			{
				entitiesAffected.push_back( (*entities)[i] );
				break;
			}
		}
	}

	std::vector<HEntity>::iterator HEit = entitiesAffected.begin();
	for ( ; HEit != entitiesAffected.end(); HEit++ )
	{
		for ( std::vector<Modifier>::iterator mod=m_Modifiers.begin(); mod!=m_Modifiers.end(); mod++ )
		{
			//CEntity* ent = *HEit;
			//debug_printf("Modifying on %ls\n", ent->m_base->m_Tag.c_str() );

			//Get the member corresponding to the javascript attribute string
			void* attribute = (char*)&**HEit + (*HEit)->m_AttributeTable[mod->attribute];
			float modValue = (invert ? -mod->value : mod->value);

			if ( varType == "int" )
				*(int*)attribute += (int)modValue;
			if ( varType == "double" )
				*(double*)attribute += (double)modValue;
			else
				*(float*)attribute += (float)modValue;
		}
	}

	for ( HEit = entitiesAffected.begin(); HEit != entitiesAffected.end(); HEit++ )
	{
		for ( std::vector<Modifier>::iterator set=m_Sets.begin(); set!=m_Sets.end(); set++ )
		{	
			//CEntity* ent = *HEit;
			//debug_printf("Setting on %ls\n", ent->m_base->m_Tag.c_str() );

			//Get the member corresponding to the javascript attribute string
			void* attribute = (char*)&**HEit + (*HEit)->m_AttributeTable[set->attribute];
			float setValue = invert ? -set->value : set->value;

			if ( varType == "int" )
				*(int*)attribute = (int)setValue;
			if ( varType == "double" )
				*(double*)attribute = (double)setValue;
			else
				*(float*)attribute = (float)setValue;
		}
	}

	if ( !first && m_effectFunction )
	{
		m_effectFunction->Run( this->GetScript() );
	}
	delete entities;
	debug_printf("Done! I think\n");
	return JS_TRUE;
}

jsval CTechnology::IsValid( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	if ( isTechValid() )
		return JS_TRUE;
	return JS_FALSE;
}
jsval CTechnology::IsExcluded( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	if ( m_excluded )
		return JS_TRUE;
	return JS_FALSE;
}
jsval CTechnology::IsResearched( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	if ( isResearched() )
		return JS_TRUE;
	return JS_FALSE;
}
inline jsval CTechnology::GetPlayerID( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( m_player->GetPlayerID() );
}
jsval CTechnology::IsJSFirst( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	if ( m_JSFirst )
		return JS_TRUE;
	return JS_FALSE;
}




