#ifndef _Network_JSEvents_H
#define _Network_JSEvents_H

#include "Network/ServerSession.h"

enum ENetworkJSEvents
{
	NET_JS_EVENT_START_GAME,
	NET_JS_EVENT_CHAT,
	NET_JS_EVENT_CONNECT_COMPLETE,
	NET_JS_EVENT_DISCONNECT,
	NET_JS_EVENT_CLIENT_CONNECT,
	NET_JS_EVENT_CLIENT_DISCONNECT,
	NET_JS_EVENT_LAST
};

class CStartGameEvent: public CScriptEvent
{
public:
	CStartGameEvent():
		CScriptEvent(L"startGame", NET_JS_EVENT_START_GAME, false)
	{}
};

class CChatEvent: public CScriptEvent
{
	CStrW m_Sender;
	CStrW m_Message;
	
public:
	CChatEvent(CStrW sender, CStrW message):
		CScriptEvent(L"chat", NET_JS_EVENT_CHAT, false ),
		m_Sender(sender),
		m_Message(message)
	{
		AddLocalProperty(L"sender", &m_Sender, true);
		AddLocalProperty(L"message", &m_Message, true);
	}
};

class CConnectCompleteEvent: public CScriptEvent
{
	CStrW m_Message;
	bool m_Success;
	
public:
	CConnectCompleteEvent(CStrW message, bool success):
		CScriptEvent(L"connectComplete", NET_JS_EVENT_CONNECT_COMPLETE, false),
		m_Message(message),
		m_Success(success)
	{
		AddLocalProperty(L"message", &m_Message, true);
		AddLocalProperty(L"success", &m_Success, true);
	}
};

class CDisconnectEvent: public CScriptEvent
{
	CStrW m_Message;
	
public:
	CDisconnectEvent(CStrW message):
		CScriptEvent(L"disconnect", NET_JS_EVENT_DISCONNECT, false),
		m_Message(message)
	{
		AddLocalProperty(L"message", &m_Message, true);
	}
};

class CClientConnectDisconnectCommon: public CScriptEvent
{
	int m_SessionID;
	CStrW m_Name;
	CNetServerSession *m_pSession;
	
public:
	CClientConnectDisconnectCommon(const wchar_t* eventName, int eventType,
			int sessionID, const CStrW &name, CNetServerSession *pSession):
		CScriptEvent(L"clientConnect", NET_JS_EVENT_CLIENT_CONNECT, false),
		m_SessionID(sessionID),
		m_Name(name),
		m_pSession(pSession)
	{
		AddLocalProperty(L"id", &m_SessionID, true);
		AddLocalProperty(L"name", &m_Name, true);
		if (m_pSession)
			AddLocalProperty(L"session", &m_pSession, true);
	}
};

struct CClientConnectEvent: public CClientConnectDisconnectCommon
{
	CClientConnectEvent(int sessionID, const CStrW &name):
		CClientConnectDisconnectCommon(
			L"clientConnect",
			NET_JS_EVENT_CLIENT_CONNECT,
			sessionID,
			name,
			NULL)
	{}

	CClientConnectEvent(CNetServerSession *pSession):
		CClientConnectDisconnectCommon(
			L"clientConnect",
			NET_JS_EVENT_CLIENT_CONNECT,
			pSession->GetID(),
			pSession->GetName(),
			pSession)
	{}
};

struct CClientDisconnectEvent: public CClientConnectDisconnectCommon
{
	CClientDisconnectEvent(int sessionID, const CStrW &name):
		CClientConnectDisconnectCommon(
			L"clientDisconnect",
			NET_JS_EVENT_CLIENT_DISCONNECT,
			sessionID,
			name,
			NULL)
	{}

	CClientDisconnectEvent(CNetServerSession *pSession):
		CClientConnectDisconnectCommon(
			L"clientDisconnect",
			NET_JS_EVENT_CLIENT_DISCONNECT,
			pSession->GetID(),
			pSession->GetName(),
			pSession)
	{}
};

#endif
