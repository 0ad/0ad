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
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);

		GetGUI()->DrawSprite(*sprite, bz, m_CachedActualSize);
	}
}
