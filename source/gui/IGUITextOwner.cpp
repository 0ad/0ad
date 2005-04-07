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
	// Delete all generated texts.
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
			Message.value == "buffer_zone")
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

void IGUITextOwner::CalculateTextPosition(CRect &ObjSize, CPos &TextPos, SGUIText &Text)
{
	EAlign align;
	EVAlign valign;
	GUI<EAlign>::GetSetting(this, "text_align", align);
	GUI<EVAlign>::GetSetting(this, "text_valign", valign);

	switch (align)
	{
	case EAlign_Left:
		TextPos.x = ObjSize.left;
		break;
	case EAlign_Center:
		// Round to integer pixel values, else the fonts look awful
		TextPos.x = floorf(ObjSize.CenterPoint().x - Text.m_Size.cx/2.f);
		break;
	case EAlign_Right:
		TextPos.x = ObjSize.right - Text.m_Size.cx;
		break;
	default:
		debug_warn("Broken EAlign in CButton::SetupText()");
		break;
	}

	switch (valign)
	{
	case EVAlign_Top:
		TextPos.y = ObjSize.top;
		break;
	case EVAlign_Center:
		// Round to integer pixel values, else the fonts look awful
		TextPos.y = floorf(ObjSize.CenterPoint().y - Text.m_Size.cy/2.f);
		break;
	case EVAlign_Bottom:
		TextPos.y = ObjSize.bottom - Text.m_Size.cy;
		break;
	default:
		debug_warn("Broken EVAlign in CButton::SetupText()");
		break;
	}
}
