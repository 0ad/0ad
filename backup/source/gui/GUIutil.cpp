/*
GUI utilities
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx.h"
#include "GUI.h"
#include "Parser.h"

using namespace std;

//--------------------------------------------------------
//  Help Classes/Structs for the GUI implementation
//--------------------------------------------------------

CClientArea::CClientArea() : pixel(0,0,0,0), percent(0,0,0,0)
{
}

CClientArea::CClientArea(const CStr &Value)
{
	SetClientArea(Value);
}

CRect CClientArea::GetClientArea(const CRect &parent) const
{
	// If it's a 0 0 100% 100% we need no calculations
	if (percent == CRect(0,0,100,100) && pixel == CRect(0,0,0,0))
		return parent;

	CRect client;

	// This should probably be cached and not calculated all the time for every object.
    client.left =	parent.left + int(float((parent.right-parent.left)*percent.left)/100.f) + pixel.left;
	client.top =	parent.top + int(float((parent.bottom-parent.top)*percent.top)/100.f) + pixel.top;
	client.right =	parent.left + int(float((parent.right-parent.left)*percent.right)/100.f) + pixel.right;
	client.bottom =	parent.top + int(float((parent.bottom-parent.top)*percent.bottom)/100.f) + pixel.bottom;

	return client;
}

bool CClientArea::SetClientArea(const CStr &Value)
{
	// Get value in STL string format
	string _Value = (const TCHAR*)Value;

	// This might lack incredible speed, but since all XML files
	//  are read at startup, reading 100 client areas will be
	//  negligible in the loading time.

	// Setup parser to parse the value
	CParser parser;

	// One of the for values:
	//  will give outputs like (in argument):
	//  (200) <== no percent, just the first $value
	//  (200) (percent) <== just the percent
	//  (200) (percent) (100) <== percent PLUS pixel
	//  (200) (percent) (-100) <== percent MINUS pixel
	//  (200) (percent) (100) (-100) <== Both PLUS and MINUS are used, INVALID
	string one_value = "_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]";
	string four_values = one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)_"; // it's easier to just end with another delimiter

	parser.InputTaskType("ClientArea", four_values);

    CParserLine line;
	line.ParseString(parser, _Value);

	if (!line.m_ParseOK)
		return false;

	int arg_count[4]; // argument counts for the four values
	int arg_start[4] = {0,0,0,0}; // location of first argument, [0] is alwasy 0

	// Divide into the four piles (delimiter is an argument named "delim")
	for (int i=0, valuenr=0; i<line.GetArgCount(); ++i)
	{
		string str;
		line.GetArgString(i, str);
		if (str == "delim")
		{
			if (valuenr==0)
			{
				arg_count[0] = i;
				arg_start[1] = i+1;
			}
			else
			{
				if (valuenr!=3)
				{
					arg_start[valuenr+1] = i+1;
					arg_count[valuenr] = arg_start[valuenr+1] - arg_start[valuenr] - 1;
				}
				else
					arg_count[3] = line.GetArgCount() - arg_start[valuenr] - 1;
			}

			++valuenr;
		}
	}

	// Iterate argument
	
	// This is the scheme:
	// 1 argument = Just pixel value
	// 2 arguments = Just percent value
	// 3 arguments = percent and pixel
	// 4 arguments = INVALID

  	// Default to 0
	int values[4][2] = {{0,0},{0,0},{0,0},{0,0}};

	for (int v=0; v<4; ++v)
	{
		if (arg_count[v] == 1)
		{
			string str;
			line.GetArgString(arg_start[v], str);

			if (!line.GetArgInt(arg_start[v], values[v][1]))
				return false;
		}
		else
		if (arg_count[v] == 2)
		{
			if (!line.GetArgInt(arg_start[v], values[v][0]))
				return false;
		}
		else
		if (arg_count[v] == 3)
		{
			if (!line.GetArgInt(arg_start[v], values[v][0]) ||
				!line.GetArgInt(arg_start[v]+2, values[v][1]))
				return false;

		}
		else return false;
	}

	// Now store the values[][] in the right place
	pixel.left =		values[0][1];
	pixel.top =			values[1][1];
	pixel.right =		values[2][1];
	pixel.bottom =		values[3][1];
	percent.left =		values[0][0];
	percent.top =		values[1][0];
	percent.right =		values[2][0];
	percent.bottom =	values[3][0];
	return true;
}

//--------------------------------------------------------
//  Utilities implementation
//--------------------------------------------------------
IGUIObject * CInternalCGUIAccessorBase::GetObjectPointer(CGUI &GUIinstance, const CStr &Object)
{
//	if (!GUIinstance.ObjectExists(Object))
//		return NULL;

	return GUIinstance.m_pAllObjects.find(Object)->second;
}

const IGUIObject * CInternalCGUIAccessorBase::GetObjectPointer(const CGUI &GUIinstance, const CStr &Object)
{
//	if (!GUIinstance.ObjectExists(Object))
//		return NULL;

	return GUIinstance.m_pAllObjects.find(Object)->second;
}

void CInternalCGUIAccessorBase::QueryResetting(IGUIObject *pObject)
{
	GUI<>::RecurseObject(0, pObject, &IGUIObject::ResetStates);
}

void * CInternalCGUIAccessorBase::GetStructPointer(IGUIObject *pObject, const EGUISettingsStruct &SettingsStruct)
{
	return pObject->GetStructPointer(SettingsStruct);
}

void CInternalCGUIAccessorBase::HandleMessage(IGUIObject *pObject, const SGUIMessage &message)
{
	pObject->HandleMessage(message);		
}
