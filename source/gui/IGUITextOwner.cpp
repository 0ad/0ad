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
		//  these. Although that is not certain, but one will have to manually
		//  change it and disregard this function.
		// TODO Gee: (2004-09-07) Make sure this is all options that can affect the text.
		// Also TODO: If several things are changing (e.g. when loading the file),
		// only call SetupText once (probably just before it's drawn)
		if (Message.value == "size" || Message.value == "z" ||
			Message.value == "absolute" || Message.value == "caption" ||
			Message.value == "font" || Message.value == "textcolor" ||
			Message.value == "buffer-zone")
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
						 const float &z, const CRect &UNUSEDPARAM(clipping))
{
	if (index < 0 || index >= (int)m_GeneratedTexts.size())
	{
		debug_warn("Trying to draw a Text Index within a IGUITextOwner that doesn't exist");
		return;
	}

	if (GetGUI())
	{
		GetGUI()->DrawText(*m_GeneratedTexts[index], color, pos, z);
	}
}
