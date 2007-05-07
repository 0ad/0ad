#ifndef INCLUDED_ATLASEVENTLOOP
#define INCLUDED_ATLASEVENTLOOP

/* Disabled (and should be removed if it turns out to be unnecessary)
- see MessagePasserImpl.cpp for information

#include "wx/evtloop.h"

struct tagMSG;

// See ScenarioEditor.cpp's QueryCallback for explanation

class AtlasEventLoop : public wxEventLoop
{
	std::vector<tagMSG*> m_Messages;
	bool m_NeedsPaint;

public:
	AtlasEventLoop();

	void AddMessage(tagMSG* msg);
	void NeedsPaint();

	virtual bool Dispatch();
};

*/

#endif // INCLUDED_ATLASEVENTLOOP
