#ifndef _PAINTOBJECTCOMMAND_H
#define _PAINTOBJECTCOMMAND_H

#include "Command.h"
#include "Matrix3D.h"
#include "Entity.h"

class CUnit;
class CBaseEntity;
class CObjectThing;

class CPaintObjectCommand : public CCommand
{
public:
	// constructor, destructor
	CPaintObjectCommand(CObjectThing* entity, const CMatrix3D& transform);
	~CPaintObjectCommand();

	// return the texture name of this command
	const char* GetName() const { return "Add Unit"; }
	
	// execute this command
	void Execute();
	
	// can undo command?
	bool IsUndoable() const { return false; }
	// undo 
	void Undo();
	// redo 
	void Redo();

	// notification that command has finished (ie object stopped rotating) - convert
	// unit to entity if there's a template for it
	void Finalize();

	void UpdateTransform(CMatrix3D& transform);

	// return unit added to world
//	CUnit* GetUnit() { return m_Unit; }

private:
	// unit to add to world
	HEntity m_Entity;
	// entity to paint
	CObjectThing* m_Thing;
	// model transformation
	CMatrix3D m_Transform;
};

#endif
