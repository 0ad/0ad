#include "precompiled.h"

#include "MessageHandlerImpl.h"

using namespace AtlasMessage;

void MessageHandlerImpl::Add(IMessage* msg)
{
	m_Mutex.Lock();

	m_Queue.push(msg);

	m_Mutex.Unlock();
}

IMessage* MessageHandlerImpl::Retrieve()
{
	m_Mutex.Lock();

	IMessage* msg = NULL;
	if (! m_Queue.empty())
	{
		msg = m_Queue.front();
		m_Queue.pop();
	}

	m_Mutex.Unlock();

	return msg;
}

void MessageHandlerImpl::Query(IMessage&)
{
}

void MessageHandlerImpl::QueryDone()
{
}

MessageHandler* g_MessageHandler = NULL;
