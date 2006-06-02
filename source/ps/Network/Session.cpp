#include "precompiled.h"

#include "Session.h"

CNetSession::~CNetSession()
{
	g_SessionManager.Deregister(this);
}
