/*==================================================================
| 
| Name: Sprite.h
|
|===================================================================
|
| Author: Ben Vinegar
| Contact: benvinegar () hotmail ! com
|
|
| Last Modified: 03/08/04
|
| Overview: Billboarding sprite class - always faces the camera. It
|           does this by getting the current model view matrix state.
|
|
| Usage: The functions speak for themselves. Instantiate, then be
|        sure to pass a loaded (using tex_load()) texture before 
|        calling Render().
|
| To do: TBA
|
| More Information: TBA
|
==================================================================*/

#ifndef SPRITE_H
#define SPRITE_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "Vector3D.h"
#include <GL/gl.h>

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

class CSprite 
{
public:
	CSprite();
	~CSprite();

	void Render();

	int SetTexture(GLuint tex);

	void SetSize(float width, float height);

	float GetWidth();
	void  SetWidth(float width);

	float GetHeight();
	void  SetHeight(float height);

	CVector3D GetTranslation();
	void SetTranslation(CVector3D pos);
	void SetTranslation(float x, float y, float z);

	CVector3D GetScale();
	void SetScale(CVector3D scale);
	void SetScale(float x, float y, float z);

	void SetColour(float * colour);
	void SetColour(float r, float g, float b, float a = 1.0f);
private:
	void BeginBillboard();
	void EndBillboard();

	GLuint      m_texture;

	CVector3D	m_coords[4];

	float	    m_width;
	float		m_height;

	CVector3D	m_translation;
	CVector3D	m_scale;

	float m_colour[4];
};

#endif // SPRITE_H