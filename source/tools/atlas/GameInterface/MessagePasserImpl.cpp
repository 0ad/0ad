#include "precompiled.h"

#include "MessagePasserImpl.h"

using namespace AtlasMessage;

void MessagePasserImpl::Add(IMessage* msg)
{
	m_Mutex.Lock();

	m_Queue.push(msg);

	m_Mutex.Unlock();
}

IMessage* MessagePasserImpl::Retrieve()
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

void MessagePasserImpl::Query(IMessage&)
{
}

void MessagePasserImpl::QueryDone()
{
}

MessagePasser* g_MessagePasser = NULL;
