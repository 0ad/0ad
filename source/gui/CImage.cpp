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
	AddSetting(GUIST_CStr,			"sprite");
}

CImage::~CImage()
{
}

void CImage::Draw() 
{
	if (GetGUI())
	{
		float bz = GetBufferedZ();

		CStr sprite;
		GUI<CStr>::GetSetting(this, "sprite", sprite);

		GetGUI()->DrawSprite(sprite, bz, m_CachedActualSize);
	}
}
