/*
COverlay
by Rich Cross, rich@0ad.wildfiregames.com


--Overview--

	Class representing 2D screen overlays; includes functionality for overlay
	position, color, texture and borders.
*/

#ifndef COVERLAY_H
#define COVERLAY_H

struct CColor
{
	CColor() {}
	CColor(float cr,float cg,float cb,float ca) : r(cr), g(cg), b(cb), a(ca) {}

	float r, g, b, a;
};

// yuck - MFC already defines a CRect class...
#define CRect PS_CRect

struct CRect
{
	CRect() {}
	CRect(int _l, int _t, int _r, int _b) :
		left(_l),
		top(_t),
		right(_r),
		bottom(_b) {}
	int bottom, top, left, right;

	bool operator ==(const CRect &rect) const
	{
		return	(bottom==rect.bottom) &&
				(top==rect.top) &&
				(left==rect.left) &&
				(right==rect.right);
	}

	bool operator !=(const CRect &rect) const
	{
		return	!(*this==rect);
	}
};


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "terrain/Texture.h"


//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * @author Rich Cross
 *
 * Overlay class definition.
 */
class COverlay
{
public:
	/**
	 * Default constructor; creates an overlay that won't actually be renderable
	 */
	COverlay();
	/**
	 * Constructor with setup for more common parameters
	 */
	COverlay(const CRect& rect,int z,const CColor& color,const char* texturename="",bool hasBorder=false,
		const CColor& bordercolor=CColor(0,0,0,0));
	/**
	 * Destructor 
	 */	
	~COverlay();

	/**
	 * Get coordinates
	 */	
	const CRect& GetRect() const { return m_Rect; }

	/**
	 * Get depth
	 */	
	int GetZ() const { return m_Z; }

	/**
	 * Get texture (not const as Renderer need to modify texture to store handle)
	 */	
	CTexture& GetTexture() { return m_Texture; }

	/**
	 * Get color
	 */	
	const CColor& GetColor() const { return m_Color; }

	/**
	 * Get border flag
	 */	
	bool HasBorder() const { return m_HasBorder; }

	/**
	 * Get border color
	 */	
	const CColor& GetBorderColor() const { return m_BorderColor; }


private:
	/// screen space coordinates of overlay
	CRect m_Rect;
	/// depth of overlay, for correctly overlapping overlays; higher z implies in-front-of behaviour
	int m_Z;
	/// texture to use in rendering overlay; can be a null texture
	CTexture m_Texture;
	/// overlay color
	CColor m_Color;
	// flag indicating whether to render overlay using a border
	bool m_HasBorder;
	/// border color
	CColor m_BorderColor;
};


#endif