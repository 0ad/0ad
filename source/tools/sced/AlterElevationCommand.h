#ifndef _ALTERELEVATIONCOMMAND_H
#define _ALTERELEVATIONCOMMAND_H

#include <set>
#include "res/res.h"
#include "Command.h"
#include "Array2D.h"


class CAlterElevationCommand : public CCommand
{
public:
	// constructor, destructor
	CAlterElevationCommand(int brushSize,int selectionCentre[2]);
	~CAlterElevationCommand();

	// execute this command
	void Execute();
	
	// can undo command?
	bool IsUndoable() const { return true; }
	// undo 
	void Undo();
	// redo 
	void Redo();

	// abstract function implemented by subclasses to fill in outgoing data member
	virtual void CalcDataOut(int x0,int x1,int z0,int z1) = 0;

protected:
	void ApplyDataToSelection(const CArray2D<u16>& data);

	// size of brush
	int m_BrushSize;
	// centre of brush
	int m_SelectionCentre[2];
	// origin of data set
	int m_SelectionOrigin[2];
	// input data (original terrain heights)
	CArray2D<u16> m_DataIn;
	// output data (final terrain heights)
	CArray2D<u16> m_DataOut;
};

#endif
