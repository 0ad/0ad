//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/28/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/include/ilu_filter.h
//
// Description: Applies filters to an image.
//
//-----------------------------------------------------------------------------


#ifndef FILTER_H
#define FILTER_H

#include "ilu_internal.h"


ILint Filters[] = {
	//   Average
	1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	9, 1,
	/*-1, -3, -1,
	-3, 41, -3,
	-1, -3, -1,
	25, 0,*/
	//	Gaussian
	1, 2, 1,
	2, 4, 2,
	1, 2, 1,
	16, 1,
	//	Horizontal Sobel
	1,  2,  1,
	0,  0,  0,
	-1, -2, -1,
	1, 0,
	//	Vertical Sobel
	1, 0, -1,
	2, 0, -2,
	1, 0, -1,
	1, 0,
	//	Horizontal Prewitt
	-1, -1, -1,
	0,  0,  0,
	1,  1,  1,
	1, 0,
	//	Vertical Prewitt
	1, 0, -1,
	1, 0, -1,
	1, 0, -1,
	1, 0,
	// Emboss
	-1, 0, 1,
	-1, 0, 1,
	-1, 0, 1,
	1, 128,
	// Emboss Edge Detect
	-1, 0, 1,
	-1, 0, 1,
	-1, 0, 1,
	1, 0
};


#endif//FILTER_H
