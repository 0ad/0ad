#ifndef ATLASEVENTLOOP_H__
#define ATLASEVENTLOOP_H__

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

#endif // ATLASEVENTLOOP_H__
