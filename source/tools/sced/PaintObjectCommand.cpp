#include "precompiled.h"

#include "PaintObjectCommand.h"
#include "UnitManager.h"
#include "ObjectEntry.h"
#include "Model.h"
#include "Unit.h"
#include "Game.h"

#include "BaseEntity.h"
#include "BaseEntityCollection.h"
#include "EntityManager.h"
#include "ObjectManager.h"

CPaintObjectCommand::CPaintObjectCommand(CObjectThing* object,const CMatrix3D& transform) 
	: m_Thing(object), m_Transform(transform), m_Entity()
{
}

CPaintObjectCommand::~CPaintObjectCommand()
{
}


void CPaintObjectCommand::Execute()
{
	m_Thing->Create(m_Transform, 1);
}

void CPaintObjectCommand::UpdateTransform(CMatrix3D& transform)
{
	m_Thing->SetTransform(transform);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Finalize: notification that command has finished (ie object stopped rotating) - convert
// unit to entity if there's a template for it
void CPaintObjectCommand::Finalize()
{	
//	CBaseEntity* templateObject = g_EntityTemplateCollection.getTemplateByActor(m_Object);
//	if( templateObject )
//	{
//		CVector3D orient = m_Unit->GetModel()->GetTransform().GetIn();
//		CVector3D position = m_Unit->GetModel()->GetTransform().GetTranslation();
//		g_UnitMan.RemoveUnit(m_Unit);
//		HEntity ent = g_EntityManager.create( templateObject, position, atan2( -orient.X, -orient.Z ) );
//		ent->SetPlayer(g_Game->GetPlayer(1));
//	}
}


void CPaintObjectCommand::Undo()
{
	// remove model from unit managers list
//	g_UnitMan.RemoveUnit(m_Unit);
}

void CPaintObjectCommand::Redo()
{
	// add the unit back to the unit manager
//	g_UnitMan.AddUnit(m_Unit);
}
