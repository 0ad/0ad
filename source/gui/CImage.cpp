/*
CImage
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "CImage.h"

#include "ogl.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CImage::CImage()
{
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_int,					"icon-id");
}

CImage::~CImage()
{
}

void CImage::Draw() 
{
	if (GetGUI())
	{
		float bz = GetBufferedZ();

		CGUISpriteInstance *sprite;
		int icon_id;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
		GUI<int>::GetSetting(this, "icon-id", icon_id);

		GetGUI()->DrawSprite(*sprite, icon_id, bz, m_CachedActualSize);
	}
}
