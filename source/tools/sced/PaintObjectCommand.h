#ifndef _PAINTOBJECTCOMMAND_H
#define _PAINTOBJECTCOMMAND_H

#include "Command.h"
#include "Matrix3D.h"

class CUnit;
class CObjectEntry;

class CPaintObjectCommand : public CCommand
{
public:
	// constructor, destructor
	CPaintObjectCommand(CObjectEntry* object,const CMatrix3D& transform);
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

	// return unit added to world
	CUnit* GetUnit() { return m_Unit; }

private:
	// unit to add to world
	CUnit* m_Unit;
	// object to paint
	CObjectEntry* m_Object;
	// model transformation
	CMatrix3D m_Transform;
};

#endif
