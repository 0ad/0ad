/*
CGUISprite
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx.h"
#include "GUI.h"

using namespace std;

void CGUISprite::Draw(const float &z, const CRect &rect, const CRect &clipping=CRect(0,0,0,0))
{
	bool DoClipping = (clipping != CRect(0,0,0,0));

	// Iterate all images and request them being drawn be the
	//  CRenderer
	std::vector<SGUIImage>::iterator it;
	for (it=m_Images.begin(); it!=m_Images.end(); ++it)
	{
		glPushMatrix();
			glTranslatef(0.0f, 0.0f, z);

			// Do this
			glBegin(GL_QUADS);
				glVertex2i(rect.right,			rect.bottom);
				glVertex2i(rect.left,			rect.bottom);
				glVertex2i(rect.left,			rect.top);
				glVertex2i(rect.right,			rect.top);
			glEnd();

		glPopMatrix();
	}
}