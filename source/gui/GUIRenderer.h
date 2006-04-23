#ifndef GUIRenderer_h
#define GUIRenderer_h

#include "lib/res/handle.h"
#include "ps/Overlay.h"
#include "ps/CStr.h"
#include <vector>

struct SGUIImageEffects;

namespace GUIRenderer
{
	class IGLState
	{
	public:
		virtual ~IGLState() {};
		virtual void Set(Handle tex)=0;
		virtual void Unset()=0;
	};

	struct SDrawCall
	{
		SDrawCall() : m_TexHandle(0), m_Effects(NULL) {}

		Handle m_TexHandle;

		bool m_EnableBlending;

		IGLState* m_Effects;

		CRect m_Vertices;
		CRect m_TexCoords;
		float m_DeltaZ;

		CColor m_BorderColor; // == CColor() for no border
		CColor m_BackColor;
	};

	class DrawCalls : public std::vector<SDrawCall>
	{
	public:
		void clear();
		DrawCalls();
		DrawCalls(const DrawCalls&);
		const DrawCalls& DrawCalls::operator=(const DrawCalls&);
		~DrawCalls();
	};
}

#include "gui/CGUISprite.h"

namespace GUIRenderer
{
	void UpdateDrawCallCache(DrawCalls &Calls, CStr &SpriteName, CRect& Size, int CellID, std::map<CStr, CGUISprite> &Sprites);

	void Draw(DrawCalls &Calls);
}

#endif // GUIRenderer_h
