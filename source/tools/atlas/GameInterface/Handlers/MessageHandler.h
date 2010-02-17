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

#ifndef INCLUDED_MESSAGEHANDLER
#define INCLUDED_MESSAGEHANDLER

#include "../Messages.h"

#include <map>
#include <string>

namespace AtlasMessage
{

typedef void (*msgHandler)(IMessage*);
typedef std::map<std::string, msgHandler> msgHandlers;
extern msgHandlers& GetMsgHandlers();

#define THINGHANDLER(prefix, expectedtype, t) \
	void f##t(prefix##t*); \
	void f##t##_wrapper(IMessage* msg) { \
		debug_assert(msg->GetType() == IMessage::expectedtype); \
		f##t (static_cast<prefix##t*>(msg)); \
	} \
	void f##t(prefix##t* msg)

#define MESSAGEHANDLER(t) THINGHANDLER(m, Message, t)
#define QUERYHANDLER(t) THINGHANDLER(q, Query, t)

}

#endif // INCLUDED_MESSAGEHANDLER
