#include "precompiled.h"

#include "PaintObjectCommand.h"
#include "UnitManager.h"
#include "ObjectEntry.h"
#include "Model.h"
#include "Unit.h"

#include "BaseEntity.h"
#include "BaseEntityCollection.h"
#include "EntityManager.h"

CPaintObjectCommand::CPaintObjectCommand(CObjectEntry* object,const CMatrix3D& transform) 
	: m_Object(object), m_Transform(transform), m_Unit(0)
{
}

CPaintObjectCommand::~CPaintObjectCommand()
{
}


void CPaintObjectCommand::Execute()
{
	// create new unit
	m_Unit=new CUnit(m_Object,m_Object->m_Model->Clone());
	m_Unit->GetModel()->SetTransform(m_Transform);

	// add this unit to list of units stored in unit manager
	g_UnitMan.AddUnit(m_Unit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Finalize: notification that command has finished (ie object stopped rotating) - convert
// unit to entity if there's a template for it
void CPaintObjectCommand::Finalize()
{	
	CBaseEntity* templateObject = g_EntityTemplateCollection.getTemplateByActor(m_Object);
	if( templateObject )
	{
		CVector3D orient = m_Unit->GetModel()->GetTransform().GetIn();
		CVector3D position = m_Unit->GetModel()->GetTransform().GetTranslation();
		g_UnitMan.RemoveUnit(m_Unit);
		g_EntityManager.create( templateObject, position, atan2( -orient.X, -orient.Z ) );
	}
}


void CPaintObjectCommand::Undo()
{
	// remove model from unit managers list
	g_UnitMan.RemoveUnit(m_Unit);
}

void CPaintObjectCommand::Redo()
{
	// add the unit back to the unit manager
	g_UnitMan.AddUnit(m_Unit);
}
