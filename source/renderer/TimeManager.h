#ifndef INCLUDED_TIMEMANAGER
#define INCLUDED_TIMEMANAGER

#include "graphics/Texture.h"
#include "ps/Overlay.h"
#include "maths/Matrix3D.h"
#include "lib/ogl.h"


class CTimeManager
{
public:
	CTimeManager();
	
	double GetFrameDelta();
	double GetGlobalTime();
	
	void Update(double delta);

private:
	double m_frameDelta;
	double m_globalTime;
};


#endif // INCLUDED_TIMEMANAGER