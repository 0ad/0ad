/*
 *-----------------------------------------------------------------------------
 *	FILE			: NetJsEvents.h
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Definitions for JavaScript events used by the network
 *					  system
 *-----------------------------------------------------------------------------
 */

#ifndef NETJSEVENTS_H
#define NETJSEVENTS_H

// INCLUDES
#include "NetSession.h"
#include "scripting/DOMEvent.h"

// DEFINES
enum NetJsEvents
{
	JS_EVENT_CLIENT_CONNECT,
	JS_EVENT_CONNECT_COMPLETE,
	JS_EVENT_CLIENT_DISCONNECT,
	JS_EVENT_DISCONNECT,
	JS_EVENT_START_GAME,
	JS_EVENT_CHAT,
	JS_EVENT_LAST
};

class CStartGameEvent: public CScriptEvent
{
public:
	CStartGameEvent():
		CScriptEvent(L"startGame", JS_EVENT_START_GAME, false)
	{}
};

class CChatEvent: public CScriptEvent
{
	CStr m_Sender;
	CStr m_Message;
	
public:
	CChatEvent(const CStr& sender, const CStr& message):
		CScriptEvent(L"chat", JS_EVENT_CHAT, false ),
		m_Sender(sender),
		m_Message(message)
	{
		AddLocalProperty(L"sender", &m_Sender, true);
		AddLocalProperty(L"message", &m_Message, true);
	}
};

class CConnectCompleteEvent: public CScriptEvent
{
	CStr m_Message;
	bool m_Success;
	
public:
	CConnectCompleteEvent(const CStr& message, bool success):
		CScriptEvent(L"connectComplete", JS_EVENT_CONNECT_COMPLETE, false),
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
	CDisconnectEvent(const CStr& message):
		CScriptEvent(L"disconnect", JS_EVENT_DISCONNECT, false),
		m_Message(message)
	{
		AddLocalProperty(L"message", &m_Message, true);
	}
};

class CClientConnectDisconnectCommon: public CScriptEvent
{
	uint		m_SessionID;
	CStr		m_Name;
	CNetSession *m_Session;
	
public:

	CClientConnectDisconnectCommon(const wchar_t* UNUSED(eventName), int UNUSED(eventType),
		int sessionID, const CStr& name, CNetSession* pSession)
		: CScriptEvent(L"clientConnect", JS_EVENT_CLIENT_CONNECT, false),
		m_SessionID(sessionID),
		m_Name(name),
		m_Session(pSession)
	{
		AddLocalProperty(L"id", &m_SessionID, true);
		AddLocalProperty(L"name", &m_Name, true);
		if (m_Session)
			AddLocalProperty(L"session", &m_Session, true);
	}
};

struct CClientConnectEvent: public CClientConnectDisconnectCommon
{
	CClientConnectEvent(int sessionID, const CStr& name):
		CClientConnectDisconnectCommon(
			L"clientConnect",
			JS_EVENT_CLIENT_CONNECT,
			sessionID,
			name,
			NULL)
	{}

	CClientConnectEvent(CNetSession *pSession):
		CClientConnectDisconnectCommon(
			L"clientConnect",
			JS_EVENT_CLIENT_CONNECT,
			pSession->GetID(),
			pSession->GetName(),
			pSession)
	{}
};

struct CClientDisconnectEvent: public CClientConnectDisconnectCommon
{
	CClientDisconnectEvent(int sessionID, const CStr& name):
		CClientConnectDisconnectCommon(
			L"clientDisconnect",
			JS_EVENT_CLIENT_DISCONNECT,
			sessionID,
			name,
			NULL)
	{}

	CClientDisconnectEvent(CNetSession *pSession):
		CClientConnectDisconnectCommon(
			L"clientDisconnect",
			JS_EVENT_CLIENT_DISCONNECT,
			pSession->GetID(),
			pSession->GetName(),
			pSession)
	{}
};

#endif	// NETJSEVENTS_H

