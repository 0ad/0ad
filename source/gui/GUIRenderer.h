#ifndef GUIRenderer_h
#define GUIRenderer_h

#include "lib/res/handle.h"
#include "ps/Overlay.h"
#include "ps/CStr.h"
#include <vector>

namespace GUIRenderer
{
	struct Handle_rfcnt_tex
	{
		Handle h;
		Handle_rfcnt_tex();
		Handle_rfcnt_tex(Handle h_);
		Handle_rfcnt_tex(const Handle_rfcnt_tex& that);
		~Handle_rfcnt_tex();
		Handle_rfcnt_tex& operator=(Handle h_);
	};

	struct SDrawCall
	{
		Handle_rfcnt_tex m_TexHandle;

		bool m_EnableBlending;

		CRect m_Vertices;
		CRect m_TexCoords;
		float m_DeltaZ;

		CColor m_BorderColor; // == CColor() for no border
		CColor m_BackColor;
	};

	typedef std::vector<SDrawCall> DrawCalls;
}

#include "gui/CGUISprite.h"

namespace GUIRenderer
{
	void UpdateDrawCallCache(DrawCalls &Calls, CStr &SpriteName, CRect& Size, int IconID, std::map<CStr, CGUISprite> &Sprites);

	void Draw(DrawCalls &Calls);
}

#endif // GUIRenderer_h
