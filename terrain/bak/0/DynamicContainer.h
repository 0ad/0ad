//***********************************************************
//
// Name:		DynamicContainer.H
// Last Update:	7/3/02
// Author:		Poya Manouchehri
//
// Description: This template class is similar to DynamicArray
//				template, except here we do not create and
//				destroy the pointer, but simply hold the
//				user created pointers of a type. ie User is
//				responsible for creating and destroying the
//				pointers;
//
//***********************************************************

#ifndef DYNAMICCONTAINER_H
#define DYNAMICCONTAINER_H

#include "Memory.H"

template<class Type>
class CDynamicContainer
{
	public:
		CDynamicContainer ();
		~CDynamicContainer ();

		//Add/Remove a pointer from the container
		void Add (Type *object);
		void Remove (Type *object);

		//Remove all pointers from the array
		void RemoveAll ();

		//Get a pointer from the array
		Type *operator [] (unsigned int index);

		int GetCount () { return m_Count; }

	protected:
		Type			**m_ppObjects; //Pointer to the array of objects

		unsigned int	m_Count;  //Number of objects in the array
};


//--------------------- Definitions

template<class Type>
CDynamicContainer<Type>::CDynamicContainer ()
{
	m_ppObjects = NULL;
}

template<class Type>
CDynamicContainer<Type>::~CDynamicContainer ()
{
	RemoveAll ();
}

template<class Type>
void CDynamicContainer<Type>::Add (Type *object)
{
	m_Count++;

	//No array already exists
	if (m_Count-1 == 0)
		m_ppObjects = (Type**)Alloc (m_Count*sizeof(Type*));
	else
		m_ppObjects = (Type**)Realloc ((void**)&m_ppObjects, m_Count*sizeof(Type*));

	m_ppObjects[m_Count-1] = object;
}

template<class Type>
void CDynamicContainer<Type>::Remove (Type *object)
{
	for (int i=0; i<m_Count; i++)
	{
		//look for a match
		if (m_ppObjects[i] == object)
		{
			m_Count--;
			//switch the last pointer with this one
			m_ppObjects[i] = m_ppObjects[m_Count];
			m_ppObjects[m_Count] = NULL;

			//shrink the array
			m_ppObjects = (Type**)Realloc ((void**)&m_ppObjects, m_Count*sizeof(Type*));

			return;
		}
	}
}

template<class Type>
void CDynamicContainer<Type>::RemoveAll ()
{
	m_Count = 0;
	FreeMem (m_ppObjects);
	m_ppObjects = NULL;
}

template<class Type>
Type *CDynamicContainer<Type>::operator [] (unsigned int index)
{
	return m_ppObjects[index];
}

#endif