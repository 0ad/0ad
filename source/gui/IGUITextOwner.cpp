/*
IGUITextOwner
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUITextOwner::IGUITextOwner()
{
}

IGUITextOwner::~IGUITextOwner()
{
	// Delete scroll-bars
	vector<SGUIText*>::iterator it;
	for (it=m_GeneratedTexts.begin(); it!=m_GeneratedTexts.end(); ++it)
	{
		delete *it;
	}
}

void IGUITextOwner::AddText(SGUIText * text)
{
	m_GeneratedTexts.push_back(text);
}

void IGUITextOwner::HandleMessage(const SGUIMessage &Message)
{
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		// Everything that can change the visual appearance.
		//  it is assumed that the text of the object will be dependent on
		//  these. But that is not certain, but one will have to manually
		//  change it and disregard this function.
		if (Message.value == CStr("size") || Message.value == CStr("z") ||
			Message.value == CStr("absolute") || Message.value == CStr("caption") ||
			Message.value == CStr("font") || Message.value == CStr("textcolor"))
		{
			SetupText();
		}
		break;

	case GUIM_LOAD:
		SetupText();
		break;

	default:
		break;
	}
}

void IGUITextOwner::Draw(const int &index, const CColor &color, const CPos &pos, 
						 const float &z, const CRect &clipping)
{
	if (index < 0 || index >= (int)m_GeneratedTexts.size())
		// janwas fixed bug here; was i < 0 && i >= size - impossible.
	{
		// TODO Gee: Warning
		return;
	}

	if (GetGUI())
	{
		GetGUI()->DrawText(*m_GeneratedTexts[index], color, pos, z);
	}
}
