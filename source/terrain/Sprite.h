//***********************************************************
//
// Name:		Sprite.h
// Last Update: 22/02/04
// Author:		Ben Vinegar
//
// Description: 3D Sprite class header.
//
//***********************************************************

#ifndef SPRITE_H
#define SPRITE_H

#include "Vector3D.h"
#include "Texture.h"

class CSprite {
	public:
		CSprite();
		~CSprite();

		void Render();

		int SetTexture(CTexture *texture);

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

		CTexture	*m_texture;

		CVector3D	m_coords[4];

		float	    m_width;
		float		m_height;

		CVector3D	m_translation;
		CVector3D	m_scale;

		float m_colour[4];
};



#endif // SPRITE_H