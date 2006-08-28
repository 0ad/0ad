#ifndef ATLASEVENTLOOP_H__
#define ATLASEVENTLOOP_H__

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

#endif // ATLASEVENTLOOP_H__
