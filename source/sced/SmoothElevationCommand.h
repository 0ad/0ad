#ifndef _SMOOTHELEVATIONCOMMAND_H
#define _SMOOTHELEVATIONCOMMAND_H

#include "AlterElevationCommand.h"

class CSmoothElevationCommand : public CAlterElevationCommand
{
public:
	// constructor, destructor
	CSmoothElevationCommand(float smoothpower,int brushSize,int selectionCentre[2]);
	~CSmoothElevationCommand();

	// return the texture name of this command
	const char* GetName() const { return "Smooth Elevation"; }
	
	// calculate output data 
	void CalcDataOut(int x0,int x1,int z0,int z1);

private:
	// smoothing power - higher powers have greater smoothing effect
	float m_SmoothPower;
};

#endif
