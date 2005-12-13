#include "precompiled.h"

#include "BaseEntity.h"
#include "ObjectManager.h"
#include "CStr.h"

#include "ps/XML/Xeromyces.h"

#include "CLogger.h"
#define LOG_CATEGORY "entity"

CBaseEntity::CBaseEntity()
{
	m_base = NULL;

	AddProperty( L"tag", &m_Tag, false );
	AddProperty( L"parent", &m_base, false );
	AddProperty( L"actions.move.speed", &m_speed );
	AddProperty( L"actions.move.turningradius", &m_turningRadius );
	AddProperty( L"actions.attack.range", &( m_melee.m_MaxRange ) );
	AddProperty( L"actions.attack.rangemin", &( m_melee.m_MinRange ) );
	AddProperty( L"actions.attack.speed", &( m_melee.m_Speed ) );
	AddProperty( L"actions.gather.range", &( m_gather.m_MaxRange ) );
	AddProperty( L"actions.gather.rangemin", &( m_gather.m_MinRange ) );
	AddProperty( L"actions.gather.speed", &( m_gather.m_Speed ) );
	AddProperty( L"actions.heal.range", &( m_heal.m_MaxRange ) );
	AddProperty( L"actions.heal.rangemin", &( m_heal.m_MinRange ) );
	AddProperty( L"actions.heal.speed", &( m_heal.m_Speed ) );
	AddProperty( L"actor", &m_actorName );
	AddProperty( L"traits.extant", &m_extant );
	AddProperty( L"traits.corpse", &m_corpse );	
	AddProperty( L"traits.health.curr", &m_healthCurr );
	AddProperty( L"traits.health.max", &m_healthMax );
	AddProperty( L"traits.health.bar_height", &m_healthBarHeight );
	AddProperty( L"traits.minimap.type", &m_minimapType );
	AddProperty( L"traits.minimap.red", &m_minimapR );
	AddProperty( L"traits.minimap.green", &m_minimapG );
	AddProperty( L"traits.minimap.blue", &m_minimapB );
	AddProperty( L"traits.anchor.type", &m_anchorType );
	AddProperty( L"traits.vision.los", &m_los );
	AddProperty( L"traits.vision.permanent", &m_permanent );

	for( int t = 0; t < EVENT_LAST; t++ )
	{
		AddProperty( EventNames[t], &m_EventHandlers[t], false );
		AddHandler( t, &m_EventHandlers[t] );
	}

	// Initialize, make life a little easier on the scriptors
	m_speed = m_turningRadius = 0.0f;
	m_extant = true; m_corpse = CStrW();

	m_bound_type = CBoundingObject::BOUND_NONE;
	m_bound_circle = NULL;
	m_bound_box = NULL;
}

CBaseEntity::~CBaseEntity()
{
	if( m_bound_box )
		delete( m_bound_box );
	if( m_bound_circle )
		delete( m_bound_circle );
}

void CBaseEntity::loadBase()
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

jsval CBaseEntity::getClassSet()
{
	STL_HASH_SET<CStrW, CStrW_hash_compare>::iterator it;
	it = m_classes.m_Set.begin();
	CStrW result = *( it++ );
	for( ; it != m_classes.m_Set.end(); it++ )
		result += L" " + *it;
	return( ToJSVal( result ) );
}

void CBaseEntity::setClassSet( jsval value )
{
	// Get the set that was passed in.
	CStr temp = ToPrimitive<CStrW>( value );
	CStr entry;
	
	m_classes.m_Added.clear();
	m_classes.m_Removed.clear();

	while( true )
	{
		long brk_sp = temp.Find( ' ' );
		long brk_cm = temp.Find( ',' );
		long brk = ( brk_sp == -1 ) ? brk_cm : ( brk_cm == -1 ) ? brk_sp : ( brk_sp < brk_cm ) ? brk_sp : brk_cm; 

		if( brk == -1 )
		{
			entry = temp;
		}
		else
		{
			entry = temp.GetSubstring( 0, brk );
			temp = temp.GetSubstring( brk + 1, temp.Length() );
		}

		if( brk != 0 )
		{
			
			if( entry[0] == '-' )
			{
				entry = entry.GetSubstring( 1, entry.Length() );
				if( entry.Length() )
					m_classes.m_Removed.push_back( entry );
			}
			else
			{	
				if( entry[0] == '+' )
					entry = entry.GetSubstring( 1, entry.Length() );
				if( entry.Length() )
					m_classes.m_Added.push_back( entry );
			}
		}		
		if( brk == -1 ) break;
	}

	rebuildClassSet();
}

void CBaseEntity::rebuildClassSet()
{
	m_classes.Rebuild();
	InheritorsList::iterator it;
	for( it = m_Inheritors.begin(); it != m_Inheritors.end(); it++ )
		(*it)->rebuildClassSet();
}

bool CBaseEntity::loadXML( CStr filename )
{
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		// Fail
		return false;

	// Define all the elements and attributes used in the XML file
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	#define AT(x) int at_##x = XeroFile.getAttributeID(#x)
	// Only the ones we can't load using normal methods.
	EL(entity);
	EL(script);
	EL(footprint);
	EL(event);
	AT(parent);
	AT(radius);
	AT(width);
	AT(depth);
	AT(height);
	AT(on);
	AT(file);
	AT(function);
	#undef AT
	#undef EL

	XMBElement Root = XeroFile.getRoot();

	if( Root.getNodeName() != el_entity )
	{
		LOG( ERROR, LOG_CATEGORY, "CBaseEntity::LoadXML: XML root was not \"Entity\" in file %s. Load failed.", filename.c_str() );
		return( false );
	}

	XMBElementList RootChildren = Root.getChildNodes();

	m_Tag = CStr(filename).AfterLast("/").BeforeLast(".xml");

	m_Base_Name = Root.getAttributes().getNamedItem( at_parent );
	
	for (int i = 0; i < RootChildren.Count; ++i)
	{
		XMBElement Child = RootChildren.item(i);

		int ChildName = Child.getNodeName();
		if( ChildName == el_script )
		{
			CStr Include = Child.getAttributes().getNamedItem( at_file );

			// TODO: Probably try and determine if this file has already been loaded, and skip it.

			if( Include.Length() )
				g_ScriptingHost.RunScript(Include);

			CStr Inline = Child.getText();
			if( Inline.Length() )
				g_ScriptingHost.RunMemScript(Inline.c_str(), Inline.Length(), filename.c_str(), Child.getLineNumber());
		}
		else if (ChildName == el_footprint)
		{
			if( Child.getAttributes().getNamedItem( at_radius ).length() )
			{
				// Specifying a circular footprint
				if( !m_bound_circle )
					m_bound_circle = new CBoundingCircle();
				CStrW radius (Child.getAttributes().getNamedItem(at_radius));
				CStrW height (Child.getAttributes().getNamedItem(at_height));
				
				m_bound_circle->setRadius( radius.ToFloat() );
				m_bound_circle->setHeight( height.ToFloat() );
				m_bound_type = CBoundingObject::BOUND_CIRCLE;
			}
			else
			{
				if( !m_bound_box )
					m_bound_box = new CBoundingBox();
				CStrW width (Child.getAttributes().getNamedItem(at_width));
				CStrW depth (Child.getAttributes().getNamedItem(at_depth));
				CStrW height (Child.getAttributes().getNamedItem(at_height));

				m_bound_box->setDimensions( width.ToFloat(), depth.ToFloat() );
				m_bound_box->setHeight( height.ToFloat() );
				m_bound_type = CBoundingObject::BOUND_OABB;
			}
		}
		else if( ChildName == el_event )
		{
			// Action...On for consistency with the GUI.
			CStrW EventName = L"on" + (CStrW)Child.getAttributes().getNamedItem( at_on );

			CStrW Code (Child.getText());
			utf16string ExternalFunction = Child.getAttributes().getNamedItem( at_function );

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
							LOG( ERROR, LOG_CATEGORY, "CBaseEntity::LoadXML: Function does not exist for event %hs in file %s. Load failed.", EventName.c_str(), filename.c_str() );
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

	return true;
}

void CBaseEntity::XMLLoadProperty( const CXeromyces& XeroFile, const XMBElement& Source, CStrW BasePropertyName )
{
	// Add a property, put the node text into it.
	CStrW PropertyName = BasePropertyName + CStr8( XeroFile.getElementString( Source.getNodeName() ) );

	IJSComplexProperty* Existing = HasProperty( PropertyName );
	if( Existing )
	{	
		if( !Existing->m_Intrinsic )
			LOG( WARNING, LOG_CATEGORY, "CBaseEntity::XMLAddProperty: %ls already defined for %ls. Property trees will be merged.", PropertyName.c_str(), m_Tag.c_str() );
		Existing->Set( this, JSParseString( Source.getText() ) );
		Existing->m_Inherited = false;
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
			AddProperty( PropertyName, Source.getText() );
	}

	
	PropertyName += CStrW( L"." );

	// Retrieve any attributes it has and add them as subproperties.
	XMBAttributeList AttributeSet = Source.getAttributes();
	for( unsigned int AttributeID = 0; AttributeID < (unsigned int)AttributeSet.Count; AttributeID++ )
	{
		XMBAttribute Attribute = AttributeSet.item( AttributeID );
		CStrW AttributeName = PropertyName + CStr8( XeroFile.getAttributeString( Attribute.Name ) );
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

void CBaseEntity::ScriptingInit()
{
	AddMethod<jsval, &CBaseEntity::ToString>( "toString", 0 );
	AddClassProperty( L"traits.id.classes", (GetFn)&CBaseEntity::getClassSet, (SetFn)&CBaseEntity::setClassSet );

	CJSComplex<CBaseEntity>::ScriptingInit( "EntityTemplate" );
}

// Script-bound functions

JSObject* CBaseEntity::GetScriptExecContext( IEventTarget* target )
{ 
	return( target->GetScriptExecContext( target ) );
}

jsval CBaseEntity::ToString( JSContext* cx, uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	wchar_t buffer[256];
	swprintf( buffer, 256, L"[object EntityTemplate: %ls]", m_Tag.c_str() );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}
