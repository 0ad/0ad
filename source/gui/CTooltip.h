/*
GUI Object - Tooltip

--Overview--

Mostly like CText, but intended for dynamic tooltips

*/

#ifndef CTooltip_H
#define CTooltip_H

#include "IGUITextOwner.h"

class CTooltip : public IGUITextOwner
{
	GUI_OBJECT(CTooltip)

public:
	CTooltip();
	virtual ~CTooltip();

protected:
	void SetupText();

	virtual void HandleMessage(const SGUIMessage &Message);

	virtual void Draw();
};

#endif
