#include "precompiled.h"

#include "CTooltip.h"
#include "CGUI.h"

CTooltip::CTooltip()
{
	AddSetting(GUIST_float,					"buffer-zone");
	AddSetting(GUIST_CGUIString,			"caption");
	AddSetting(GUIST_CStr,					"font");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_float,					"time");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_int,					"maxwidth");
	AddSetting(GUIST_CPos,					"pos");
	AddSetting(GUIST_EVAlign,				"anchor");

	AddSetting(GUIST_CPos,					"_mousepos");

	GUI<float>::SetSetting(this, "time", 0.5f);
	GUI<EVAlign>::SetSetting(this, "anchor", EVAlign_Bottom);

	// Set up a blank piece of text, to be replaced with a more
	// interesting message later
	AddText(new SGUIText());
}

CTooltip::~CTooltip()
{
}

void CTooltip::SetupText()
{
	if (!GetGUI())
		return;

	assert(m_GeneratedTexts.size()==1);

	CStr font;
	if (GUI<CStr>::GetSetting(this, "font", font) != PS_OK || font.Length()==0)
		font = "default";

    float buffer_zone=0.f;
	GUI<float>::GetSetting(this, "buffer-zone", buffer_zone);

	CGUIString caption;
	GUI<CGUIString>::GetSetting(this, "caption", caption);

	float max_width = 500.f; // TODO: max-width setting

	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, font, max_width, buffer_zone, this);

	CPos mousepos, pos;
	EVAlign anchor;
	GUI<CPos>::GetSetting(this, "_mousepos", mousepos);
	GUI<CPos>::GetSetting(this, "pos", pos);
	GUI<EVAlign>::GetSetting(this, "anchor", anchor);

	// Position the tooltip relative to the mouse

	CClientArea size;
	size.pixel.left = mousepos.x + pos.x;
	size.pixel.right = size.pixel.left + m_GeneratedTexts[0]->m_Size.cx;
	switch (anchor)
	{
	case EVAlign_Top:
		size.pixel.top = mousepos.y + pos.y;
		size.pixel.bottom = size.pixel.top + m_GeneratedTexts[0]->m_Size.cy;
		break;
	case EVAlign_Bottom:
		size.pixel.bottom = mousepos.y + pos.y;
		size.pixel.top = size.pixel.bottom - m_GeneratedTexts[0]->m_Size.cy;
		break;
	case EVAlign_Center:
		size.pixel.top = mousepos.y + pos.y - m_GeneratedTexts[0]->m_Size.cy/2.f;
		size.pixel.bottom = size.pixel.top + m_GeneratedTexts[0]->m_Size.cy;
		break;
	default:
		debug_warn("Invalid EVAlign!");
	}

	// Adjust it if it's falling off the screen

	extern int g_xres, g_yres;
	float screenw = (float)g_xres, screenh = (float)g_yres;

	if (size.pixel.top < 0.f)
		size.pixel.bottom -= size.pixel.top, size.pixel.top = 0.f;
	else if (size.pixel.bottom > screenh)
		size.pixel.top -= (size.pixel.bottom-screenh), size.pixel.bottom = screenh;
	else if (size.pixel.left < 0.f)
		size.pixel.right -= size.pixel.left, size.pixel.left = 0.f;
	else if (size.pixel.right > screenw)
		size.pixel.left -= (size.pixel.right-screenw), size.pixel.right = screenw;
	
	GUI<CClientArea>::SetSetting(this, "size", size);
	UpdateCachedSize();
}

void CTooltip::HandleMessage(const SGUIMessage &Message)
{
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		// Don't update the text when the size changes, because the size is
		// changed whenever the text is updated ( => infinite recursion)
		if (/*Message.value == "size" ||*/ Message.value == "caption" ||
			Message.value == "font" || Message.value == "buffer-zone")
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

void CTooltip::Draw() 
{
	float z = 900.f; // TODO: Find a nicer way of putting the tooltip on top of everything else

	if (GetGUI())
	{
		CGUISpriteInstance *sprite;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);

		GetGUI()->DrawSprite(*sprite, 0, z, m_CachedActualSize);

		CColor color;
		GUI<CColor>::GetSetting(this, "textcolor", color);

		// Draw text
		IGUITextOwner::Draw(0, color, m_CachedActualSize.TopLeft(), z+0.1f);
	}
}
