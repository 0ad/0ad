/*
CProgressBar
*/

#include "precompiled.h"
#include "GUI.h"
#include "CProgressBar.h"

#include "lib/ogl.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CProgressBar::CProgressBar()
{
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_background");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_bar");
	AddSetting(GUIST_float,					"caption"); // aka value from 0 to 100
	AddSetting(GUIST_CStr,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip_style");
}

CProgressBar::~CProgressBar()
{
}

void CProgressBar::HandleMessage(const SGUIMessage &Message)
{
	// Important
	IGUIObject::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		// Update scroll-bar
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
		if (Message.value == CStr("caption"))
		{
			float value;
			GUI<float>::GetSetting(this, "caption", value);
			if (value > 100.f)
				GUI<float>::SetSetting(this, "caption", 100.f);
			else
			if (value < 0.f)
				GUI<float>::SetSetting(this, "caption", 0.f);
		}
		break;
	default:
		break;
	}
}

void CProgressBar::Draw() 
{
	if (GetGUI())
	{
		float bz = GetBufferedZ();

		CGUISpriteInstance *sprite_background, *sprite_bar;
		int cell_id = 0;
		float value = 0;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_background", sprite_background);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_bar", sprite_bar);
		GUI<int>::GetSetting(this, "cell_id", cell_id);
		GUI<float>::GetSetting(this, "caption", value);

		GetGUI()->DrawSprite(*sprite_background, cell_id, bz, m_CachedActualSize);


		// Get size of bar (notice it is drawn slightly closer, to appear above the background)
		CRect bar_size(m_CachedActualSize.left, m_CachedActualSize.top,
					   m_CachedActualSize.left+m_CachedActualSize.GetWidth()*(value/100.f), m_CachedActualSize.bottom);
		GetGUI()->DrawSprite(*sprite_bar, cell_id, bz+0.01f, bar_size);
	}
}
