//***********************************************************
//
// Name:		DynamicArray.H
// Last Update:	2/3/02
// Author:		Poya Manouchehri
//
// Description: This is a template class which provides an
//				an interface for a dynamic array of any
//				type.  ie new objects of the same type maybe
//				added, or can be removed. For speed, this
//				class does not support sorting.
//
//***********************************************************

#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include "Memory.H"
#include "Types.H"

template<class Type>
class CDynamicArray
{
	public:
		CDynamicArray ();
		~CDynamicArray ();

		//Add new object(s) to the array, and return the pointer
		//to the first one.
		Type *New (unsigned int count);
		//Delete an object from the array
		void Delete (Type *object);

		//Clear the entire array
		void Clear ();

		//For getting an object from the array
		inline Type &operator[] (int index);

		//Get a pointer from the array
		Type *GetPointer (int index);

		//Get the size of the array
		int GetCount () { return m_Count; }

	protected:
		unsigned int	m_Count;  //Number of objects in the array

		Type			**m_ppObjects; //Double pointer to the object array
};


//--------------------- Definitions

template<class Type>
CDynamicArray<Type>::CDynamicArray ()
{
	m_ppObjects = NULL;
	m_Count = 0;
}

template<class Type>
CDynamicArray<Type>::~CDynamicArray ()
{
	Clear ();
}

template<class Type>
Type *CDynamicArray<Type>::New (unsigned int count)
{
	Type *pBlock;
	int OldCount = m_Count;

	//No array already exists
	if (m_Count == 0)
	{
		m_Count = count;
		m_ppObjects = (Type**)Alloc (count*sizeof(Type*));
	}
	else
	{
		m_Count += count;
		m_ppObjects = (Type**)Realloc ((void*)&m_ppObjects, count*sizeof(Type*));
	}

	//create the required block
	pBlock = new Type[count];

	//assign the pointers
	for (unsigned int i=0; i<m_Count; i++)
		m_ppObjects[i] = pBlock + i;

	//return the first pointer
	return pBlock;
}

template<class Type>
void CDynamicArray<Type>::Delete (Type *object)
{
	if (m_Count == 0 || object == NULL)
		return;

	//find the object we want
	for (int i=0; i<m_Count; i++)
	{
		if (m_ppObjects[i] == object)
		{
			//swap the last element with this
			m_ppObjects[i] = m_ppObjects[m_Count-1];
			//destroy it
			delete object;
			object = NULL;
			
			m_Count--;

			if (m_Count == 0)
			{
				FreeMem ((void*)&m_ppObjects);
				m_ppObjects = NULL;
			}
			else
				m_ppObjects = (Type**)Realloc ((void*)&m_ppObjects, m_Count);

			//we won't have another match
			return;
		}
	}
}

template<class Type>
void CDynamicArray<Type>::Clear ()
{
	if (m_Count == 0)
		return;

	//delete each pointer distinctively
	for (unsigned int i=0; i<m_Count; i++)
	{
		delete m_ppObjects[i];
		m_ppObjects[i] = NULL;
	}

	//free the array
	FreeMem ((void*)&m_ppObjects);
	m_ppObjects = NULL;

	m_Count = 0;
}

template<class Type>
Type *CDynamicArray<Type>::GetPointer (int index)
{
	if (index < 0 || index >= (int)m_Count)
		return NULL;

	return m_ppObjects[index];
}

template<class Type>
Type &CDynamicArray<Type>::operator [] (int index)
{

	return *GetPointer(index);
}

#endif