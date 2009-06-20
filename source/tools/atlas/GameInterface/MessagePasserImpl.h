/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MessagePasser.h"

#include "ps/ThreadUtil.h"
#include "ps/CStr.h"
#include <queue>

class MessagePasserImpl : public AtlasMessage::MessagePasser
{
	NONCOPYABLE(MessagePasserImpl);
public:
	MessagePasserImpl();
	~MessagePasserImpl();
	virtual void Add(AtlasMessage::IMessage* msg);
	virtual AtlasMessage::IMessage* Retrieve();
	virtual void Query(AtlasMessage::QueryMessage* qry, void(*timeoutCallback)());

	bool IsEmpty();

	void SetTrace(bool t);

private:
	CMutex m_Mutex;
	CStr m_SemaphoreName;
	sem_t* m_Semaphore;
	std::queue<AtlasMessage::IMessage*> m_Queue;
	bool m_Trace;
};
