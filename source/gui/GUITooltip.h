#ifndef GUITooltip_H
#define GUITooltip_H

class IGUIObject;
class CGUI;
class CStr;

#include "Overlay.h"

class GUITooltip
{
public:
	GUITooltip();
	void Update(IGUIObject* Nearest, CPos MousePos, CGUI* GUI);

private:
	void ShowTooltip(IGUIObject* obj, CPos pos, CStr& style, CGUI* gui);

	int m_State;

	IGUIObject* m_PreviousObject;
	CStr m_PreviousTooltipName;
	CPos m_PreviousMousePos;
	double m_Time;
};

#endif // GUITooltip_H
