//***********************************************************
//
// Name:		Memory.H
// Last Update:	2/3/02
// Author:		Poya Manouchehri
//
// Description: Simple Functions for memory alloc/reallocation
//
//***********************************************************

#ifndef MEMORY_H
#define MEMORY_H

#include <windows.h>

//Allocates memory
extern void *Alloc (unsigned int size);
//Reallocates a chunk of memory
extern void *Realloc (void *ptr, unsigned int size);
//Frees the memory for allocation
extern void FreeMem (void *ptr);

#endif