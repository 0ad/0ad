#ifndef _Network_JSEvents_H
#define _Network_JSEvents_H

enum ENetworkJSEvents
{
	NET_JS_EVENT_START_GAME,
	NET_JS_EVENT_CHAT,
	NET_JS_EVENT_CONNECT_COMPLETE,
	NET_JS_EVENT_DISCONNECT,
	NET_JS_EVENT_LAST
};

class CStartGameEvent: public CScriptEvent
{
public:
	CStartGameEvent():
		CScriptEvent(L"startGame", false, NET_JS_EVENT_START_GAME)
	{}
};

class CChatEvent: public CScriptEvent
{
	CStrW m_Sender;
	CStrW m_Message;
	
public:
	CChatEvent(CStrW sender, CStrW message):
		CScriptEvent(L"chat", false, NET_JS_EVENT_CHAT),
		m_Sender(sender),
		m_Message(message)
	{
		AddReadOnlyProperty(L"sender", &m_Sender);
		AddReadOnlyProperty(L"message", &m_Message);
	}
};

class CConnectCompleteEvent: public CScriptEvent
{
	CStrW m_Message;
	bool m_Success;
	
public:
	CConnectCompleteEvent(CStrW message, bool success):
		CScriptEvent(L"connectComplete", false, NET_JS_EVENT_CONNECT_COMPLETE),
		m_Message(message),
		m_Success(success)
	{
		AddReadOnlyProperty(L"message", &m_Message);
		AddReadOnlyProperty(L"success", &m_Success);
	}
};

class CDisconnectEvent: public CScriptEvent
{
	CStrW m_Message;
	
public:
	CDisconnectEvent(CStrW message):
		CScriptEvent(L"disconnect", false, NET_JS_EVENT_DISCONNECT),
		m_Message(message)
	{
		AddReadOnlyProperty(L"message", &m_Message);
	}
};

#endif
