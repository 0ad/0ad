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

#ifndef INCLUDED_MESSAGEPASSER
#define INCLUDED_MESSAGEPASSER

#include "SharedMemory.h"

namespace AtlasMessage
{

struct IMessage;
struct QueryMessage;

class MessagePasser
{
public:
	virtual ~MessagePasser() {}

	virtual void Add(IMessage*) = 0;
		// takes ownership of IMessage object

	virtual void Query(QueryMessage*, void(*timeoutCallback)()) = 0;
		// blocks; caller retains ownership of QueryMessage object
};

extern MessagePasser* g_MessagePasser;

#define POST_MESSAGE(type, data) AtlasMessage::g_MessagePasser->Add(SHAREABLE_NEW(AtlasMessage::m##type, data))

}

#endif // INCLUDED_MESSAGEPASSER
