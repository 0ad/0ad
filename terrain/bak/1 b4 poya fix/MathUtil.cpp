//***********************************************************
//
// Name:		MathUtil.Cpp
// Last Update:	28/1/02
// Author:		Poya Manouchehri
//
// Description: This file contains some maths related
//				utility macros and fucntions.
//
//***********************************************************
unsigned int F2DW (float f) 
{
	return *((unsigned int*)&f);
}