//***********************************************************
//
// Name:		Memory.Cpp
// Last Update:	2/3/02
// Author:		Poya Manouchehri
//
// Description: Simple Functions for memory alloc/reallocation
//
//***********************************************************

#include "Memory.H"

//Allocates memory
void *Alloc (unsigned int size)
{
	//no memory allocation required
	if (size == 0)
		return NULL;

	return malloc (size);
}

//Reallocates a chunk of memory
void *Realloc (void *ptr, unsigned int size)
{
	//allocate new memory
	if (ptr == NULL)
		return Alloc (size);

	//shrink to zero, ie free all of the memory
	if (size == 0)
		free (ptr);

	return realloc (ptr, size);
}

//Frees the memory for allocation
void FreeMem (void *ptr)
{
	free (ptr);
}
