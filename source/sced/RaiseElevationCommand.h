#ifndef _RAISEELEVATIONCOMMAND_H
#define _RAISEELEVATIONCOMMAND_H

#include "AlterElevationCommand.h"

class CRaiseElevationCommand : public CAlterElevationCommand
{
public:
	// constructor, destructor
	CRaiseElevationCommand(int deltaheight,int brushSize,int selectionCentre[2]);
	~CRaiseElevationCommand();

	// return the texture name of this command
	const char* GetName() const { return m_DeltaHeight<0 ? "Lower Elevation" : "Raise Elevation"; }
	
	// calculate output data 
	void CalcDataOut(int x0,int x1,int z0,int z1);

private:
	// change in elevation, signed
	int m_DeltaHeight;
};

#endif
