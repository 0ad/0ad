#include "precompiled.h"

#include "BaseEntity.h"
#include "ObjectManager.h"
#include "CStr.h"

#include "ps/Xeromyces.h"

CBaseEntity::CBaseEntity()
{
	m_base = NULL;
	m_base.associate( this, L"super" );
	m_name.associate( this, L"name" );
	m_speed.associate( this, L"speed" );
	m_turningRadius.associate( this, L"turningRadius" );
	
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

bool CBaseEntity::loadXML( CStr filename )
{
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		// Fail
		return false;

	// Define all the elements and attributes used in the XML file
	#define EL(x) int el_##x = XeroFile.getElementID(#x)
	#define AT(x) int at_##x = XeroFile.getAttributeID(#x)
	EL(entity);
	EL(name);
	EL(actor);
	EL(speed);
	EL(turningradius);
	EL(size);
	EL(footprint);
	EL(graphicsoffset);
	AT(radius);
	AT(width);
	AT(height);
	AT(x);
	AT(y);
	#undef AT
	#undef EL

	XMBElement Root = XeroFile.getRoot();

	assert(Root.getNodeName() == el_entity);

	XMBElementList RootChildren = Root.getChildNodes();

	for (int i = 0; i < RootChildren.Count; ++i)
	{
		XMBElement Child = RootChildren.item(i);

		int ChildName = Child.getNodeName();

		if (ChildName == el_name)
		{
			m_name = (CStrW)Child.getText();
		}
		else if (ChildName == el_actor) 
		{
			m_actorObject = g_ObjMan.FindObject( (CStr)Child.getText() );
		}
		else if (ChildName == el_speed)
		{
			m_speed = CStrW(Child.getText()).ToFloat();
		}
		else if (ChildName == el_turningradius)
		{
			m_turningRadius = CStrW(Child.getText()).ToFloat();
		}
		else if (ChildName == el_size)
		{
			if( !m_bound_circle )
				m_bound_circle = new CBoundingCircle();
			CStrW radius (Child.getAttributes().getNamedItem(at_radius));
			m_bound_circle->setRadius( radius.ToFloat() );
			m_bound_type = CBoundingObject::BOUND_CIRCLE;
		}
		else if (ChildName == el_footprint)
		{
			if( !m_bound_box )
				m_bound_box = new CBoundingBox();
			CStrW width (Child.getAttributes().getNamedItem(at_width));
			CStrW height (Child.getAttributes().getNamedItem(at_height));

			m_bound_box->setDimensions( width.ToFloat(), height.ToFloat() );
			m_bound_type = CBoundingObject::BOUND_OABB;
		}
		else if (ChildName == el_graphicsoffset)
		{
			CStrW x (Child.getAttributes().getNamedItem(at_x));
			CStrW y (Child.getAttributes().getNamedItem(at_y));

			m_graphicsOffset.x = x.ToFloat();
			m_graphicsOffset.y = y.ToFloat();
		}
	}		

	return true;
}
