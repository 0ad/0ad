#include "precompiled.h"

#include <Network/Session.h>

CNetSession::~CNetSession()
{
	g_SessionManager.Deregister(this);
}
