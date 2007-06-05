#include "precompiled.h"

/* Disabled (and should be removed if it turns out to be unnecessary)
- see MessagePasserImpl.cpp for information

#include "AtlasEventLoop.h"

AtlasEventLoop::AtlasEventLoop()
: m_NeedsPaint(false)
{
}

void AtlasEventLoop::AddMessage(MSG* msg)
{
	m_Messages.push_back(msg);
}

void AtlasEventLoop::NeedsPaint()
{
	m_NeedsPaint = true;
}

bool AtlasEventLoop::Dispatch()
{
	// Process the messages that QueryCallback collected
	for (size_t i = 0; i < m_Messages.size(); ++i)
	{
		MSG* pMsg = m_Messages[i];
		wxEventLoop::GetActive()->ProcessMessage(pMsg);
		delete pMsg;
	}
	m_Messages.clear();

	if (m_NeedsPaint && wxTheApp && wxTheApp->GetTopWindow())
	{
		wxTheApp->GetTopWindow()->Refresh();
		m_NeedsPaint = false;
	}

	return wxEventLoop::Dispatch();
}
*/
