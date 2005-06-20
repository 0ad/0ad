/*
COverlay
by Rich Cross, rich@0ad.wildfiregames.com


--Overview--

	Class representing 2D screen overlays; includes functionality for overlay
	position, color, texture and borders.
*/

#ifndef COVERLAY_H
#define COVERLAY_H

#include "types.h"

class CStr;

struct CColor
{
	CColor() : r(-1.f), g(-1.f), b(-1.f), a(1.f) {}
	CColor(float cr,float cg,float cb,float ca) : r(cr), g(cg), b(cb), a(ca) {}

	bool ParseString(const CStr& Value, float DefaultAlpha);

	bool operator == (const CColor &color) const;

	bool operator != (const CColor &color) const
	{
		return !(*this==color);
	}

	// For passing to glColor[34]fv:
	const float* FloatArray() const { return &r; }

	// For passing to CRenderer:
	const u32 Int() const
	{
		return (((int)(a*255.0) & 0xff) << 24)
			 + (((int)(b*255.0) & 0xff) << 16)
			 + (((int)(g*255.0) & 0xff) <<  8)
			 + (((int)(r*255.0) & 0xff));
	}

	float r, g, b, a;
};

// yuck - MFC already defines CRect/CSize classes ...
#define CRect PS_CRect
#define CSize PS_CSize

class CPos;
class CSize;


/**
 * @author Gustav Larsson
 *
 * Rectangle class used for screen rectangles. It's very similar to the MS
 * CRect, but with FLOATS because it's meant to be used with OpenGL which
 * takes float values.
 *
 * Changed to floats 2004-08-31 /GL
 */
class CRect
{
public:
	CRect();
	CRect(const CPos &pos);
	CRect(const CSize &size);
	CRect(const CPos &upperleft, const CPos &bottomright);
	CRect(const CPos &pos, const CSize &size);
	CRect(const float &_l, const float &_t, const float &_r, const float &_b);

	// Operators
	void				operator =  (const CRect& a);
	bool				operator == (const CRect& a) const;
	bool				operator != (const CRect& a) const;
	CRect				operator -	(void) const;
	CRect				operator +	(void) const;

	CRect				operator +  (const CRect& a) const;
	CRect				operator +  (const CPos& a) const;
	CRect				operator +  (const CSize& a) const;
	CRect				operator -  (const CRect& a) const;
	CRect				operator -  (const CPos& a) const;
	CRect				operator -  (const CSize& a) const;

	void				operator += (const CRect& a);
	void				operator += (const CPos& a);
	void				operator += (const CSize& a);
	void				operator -= (const CRect& a);
	void				operator -= (const CPos& a);
	void				operator -= (const CSize& a);

	/**
	 * @return Width of Rectangle
	 */
	float GetWidth() const;
	
	/**
	 * @return Height of Rectangle
	 */
	float GetHeight() const;

	/**
	 * Get Size
	 */
	CSize GetSize() const;

	/**
	 * Get Position equivalent to top/left corner
	 */
	CPos TopLeft() const;

	/**
	 * Get Position equivalent to bottom/right corner
	 */
	CPos BottomRight() const;
	
	/**
	 * Get Position equivalent to the center of the rectangle
	 */
	CPos CenterPoint() const;

	/**
	 * Evalutates if point is within the rectangle
	 * @param point CPos representing point
	 * @return true if inside.
	 */
	bool PointInside(const CPos &point) const;

	CRect Scale(float x, float y) const;

	/**
	 * Returning CPos representing each corner.
	 */

public:
	/**
	 * Dimensions
	 */
	float left, top, right, bottom;
};

/**
 * @author Gustav Larsson
 *
 * Made to represent screen positions and delta values.
 * @see CRect
 * @see CSize
 */
class CPos
{
public:
	CPos();
	CPos(const CSize &pos);
	CPos(const float &_x, const float &_y);

	// Operators
	void				operator =  (const CPos& a);
	bool				operator == (const CPos& a) const;
	bool				operator != (const CPos& a) const;
	CPos				operator -	(void) const;
	CPos				operator +	(void) const;

	CPos				operator +  (const CPos& a) const;
	CPos				operator +  (const CSize& a) const;
	CPos				operator -  (const CPos& a) const;
	CPos				operator -  (const CSize& a) const;

	void				operator += (const CPos& a);
	void				operator += (const CSize& a);
	void				operator -= (const CPos& a);
	void				operator -= (const CSize& a);

public:
	/**
	 * Position
	 */
	float x, y;
};

/**
 * @author Gustav Larsson
 *
 * Made to represent a screen size, should in philosophy
 * be made of unsigned ints, but for the sake of compatibility
 * with CRect and CPos it's not.
 * @see CRect
 * @see CPos
 */
class CSize
{
public:
	CSize();
	CSize(const CRect &rect);
	CSize(const CPos &pos);
	CSize(const float &_cx, const float &_cy);

	// Operators
	void				operator =  (const CSize& a);
	bool				operator == (const CSize& a) const;
	bool				operator != (const CSize& a) const;
	CSize				operator -	(void) const;
	CSize				operator +	(void) const;

	CSize				operator +  (const CSize& a) const;
	CSize				operator -  (const CSize& a) const;
	CSize				operator /  (const float &a) const;
	CSize				operator *  (const float &a) const;

	void				operator += (const CSize& a);
	void				operator -= (const CSize& a);
	void				operator /= (const float& a);
	void				operator *= (const float& a);

public:
	/**
	 * Size
	 */
	float cx, cy;
};


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "Texture.h"


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
