#include "precompiled.h"

#include "BaseEntity.h"
#include "ObjectManager.h"
#include "CStr.h"

#include "ps/Xeromyces.h"

CBaseEntity::CBaseEntity()
{
	m_base = NULL;
	m_base.associate( this, "super" );
	m_name.associate( this, "name" );
	m_speed.associate( this, "speed" );
	m_turningRadius.associate( this, "turningRadius" );
	
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
	try
	{
		XeroFile.Load(filename);
	}
	catch (...)
	{
		return false;
	}

	// Define all the elements and attributes used in the XML file
	#define EL(x) int el_##x = XeroFile.getElementID(L#x)
	#define AT(x) int at_##x = XeroFile.getAttributeID(L#x)
	EL(entity);
	EL(name);
	EL(actor);
	EL(speed);
	EL(turningradius);
	EL(size);
	EL(footprint);
	EL(boundsoffset);
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
			m_name = tocstr(Child.getText());
		}
		else if (ChildName == el_actor) 
		{
			m_actorObject = g_ObjMan.FindObject( tocstr(Child.getText()) );
		}
		else if (ChildName == el_speed)
		{
			m_speed = CStr16(Child.getText()).ToFloat();
		}
		else if (ChildName == el_turningradius)
		{
			m_turningRadius = CStr16(Child.getText()).ToFloat();
		}
		else if (ChildName == el_size)
		{
			if( !m_bound_circle )
				m_bound_circle = new CBoundingCircle();
			CStr16 radius = Child.getAttributes().getNamedItem(at_radius);
			m_bound_circle->setRadius( radius.ToFloat() );
			m_bound_type = CBoundingObject::BOUND_CIRCLE;
		}
		else if (ChildName == el_footprint)
		{
			if( !m_bound_box )
				m_bound_box = new CBoundingBox();
			CStr16 width = Child.getAttributes().getNamedItem(at_width);
			CStr16 height = Child.getAttributes().getNamedItem(at_height);

			m_bound_box->setDimensions( width.ToFloat(), height.ToFloat() );
			m_bound_type = CBoundingObject::BOUND_OABB;
		}
		else if (ChildName == el_boundsoffset)
		{
			CStr16 x = Child.getAttributes().getNamedItem(at_x);
			CStr16 y = Child.getAttributes().getNamedItem(at_y);

			if( !m_bound_circle )
				m_bound_circle = new CBoundingCircle();
			if( !m_bound_box )
				m_bound_box = new CBoundingBox();

			m_bound_circle->m_offset.x = x.ToFloat();
			m_bound_circle->m_offset.y = y.ToFloat();
			m_bound_box->m_offset.x = x.ToFloat();
			m_bound_box->m_offset.y = y.ToFloat();

		}
	}		

	return true;
}
