/*
GUI utilities
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx.h"
#include "GUI.h"

using namespace std;

CGUIObject * CInternalCGUIAccessorBase::GetObjectPointer(CGUI &GUIinstance, const CStr &Object)
{
//	if (!GUIinstance.ObjectExists(Object))
//		return NULL;

	return GUIinstance.m_pAllObjects.find(Object)->second;
}

const CGUIObject * CInternalCGUIAccessorBase::GetObjectPointer(const CGUI &GUIinstance, const CStr &Object)
{
//	if (!GUIinstance.ObjectExists(Object))
//		return NULL;

	return GUIinstance.m_pAllObjects.find(Object)->second;
}