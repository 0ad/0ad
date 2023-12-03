/* Copyright (C) 2019 Wildfire Games.
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

// Used by Messages.h, so that file stays relatively clean.

#ifndef MESSAGESSETUP_NOTFIRST
#define MESSAGESSETUP_NOTFIRST

#include "MessagePasser.h"
#include "SharedTypes.h"
#include "Shareable.h"

// Structures in this file are passed over the DLL boundary, so some
// carefulness and/or luck is required...

namespace AtlasMessage
{

struct IMessage
{
	virtual const char* GetName() const = 0;
	virtual ~IMessage() {}

	enum Type { Message, Query };
	virtual Type GetType() const = 0;
};


#define MESSAGESTRUCT(t) \
	struct m##t : public IMessage { \
		virtual const char* GetName() const { return #t; } \
		virtual Type GetType() const { return IMessage::Message; } \
	private: \
		const m##t& operator=(const m##t&); \
	public:

// Messages for doing/undoing/etc world-altering commands

MESSAGESTRUCT(WorldCommand)
	mWorldCommand() {}
	virtual void* CloneData() const = 0;
	virtual bool IsMergeable() const = 0;
};
MESSAGESTRUCT(DoCommand)
	mDoCommand(mWorldCommand* c) : name(c->GetName()), data(c->CloneData()) {}
	const Shareable<std::string> name;
	const Shareable<void*> data;
		// 'data' gets deallocated by ~cWhatever in the game thread
};
MESSAGESTRUCT(UndoCommand)  };
MESSAGESTRUCT(RedoCommand)  };
MESSAGESTRUCT(MergeCommand) };


struct QueryMessage : public IMessage
{
	Type GetType() const { return IMessage::Query; }
	void Post(); // defined in ScenarioEditor.cpp

	void* m_Semaphore; // for use by MessagePasser implementations (yay encapsulation)
};


#define QUERYSTRUCT(t) \
	struct q##t : public QueryMessage { \
		const char* GetName() const { return #t; } \
	private: \
		const q##t& operator=(const q##t&); \
	public:



const bool MERGE = true;
const bool NOMERGE = false;


#define COMMANDDATASTRUCT(t) \
	struct d##t { \
	private: \
		d##t& operator=(const d##t&) = delete; \
	public:

#define COMMANDSTRUCT(t, merge) \
	struct m##t : public mWorldCommand, public d##t { \
		m##t(const d##t& d) : d##t(d) {} \
		const char* GetName() const { return #t; } \
		virtual bool IsMergeable() const { return merge; } \
		void* CloneData() const { return SHAREABLE_NEW(d##t, (*this)); } \
	private: \
		const m##t& operator=(const m##t&);\
	}



#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/comparison/equal.hpp>

#define B_TYPE(elem) BOOST_PP_TUPLE_ELEM(2, 0, elem)
#define B_NAME(elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)
#define B_CONSTRUCTORARGS(r, data, n, elem) BOOST_PP_COMMA_IF(n) B_TYPE(elem) BOOST_PP_CAT(B_NAME(elem),_)
#define B_CONSTRUCTORTYPES(r, data, n, elem) BOOST_PP_COMMA_IF(n) B_TYPE(elem)
#define B_CONSTRUCTORINIT(r, data, n, elem) BOOST_PP_COMMA_IF(n) B_NAME(elem)(BOOST_PP_CAT(B_NAME(elem),_))
#define B_CONSTMEMBERS(r, data, n, elem) const Shareable< B_TYPE(elem) > B_NAME(elem);
#define B_MEMBERS(r, data, n, elem) Shareable< B_TYPE(elem) > B_NAME(elem);

/* For each message type, generate something roughly like:
	struct mBlah : public IMessage {
		const char* GetName() const { return "Blah"; }
		mBlah(int in0_, bool in1_) : in0(in0_), in1(in1_) {}
		static mBlah* CtorType (int, bool) { return NULL; } // This doesn't do anything useful - it's just to make template-writing easier
		const Shareable<int> in0;
		const Shareable<bool> in1;
	}
*/

#define MESSAGE_WITH_INPUTS(name, vals) \
	MESSAGESTRUCT(name) \
		m##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, vals) ) \
			: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, vals) {} \
		static m##name* CtorType( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORTYPES, ~, vals) ) { return NULL; } \
		BOOST_PP_SEQ_FOR_EACH_I(B_CONSTMEMBERS, ~, vals) \
	}

#define MESSAGE_WITHOUT_INPUTS(name, vals) \
	MESSAGESTRUCT(name) \
		m##name() {} \
		static m##name* CtorType() { return NULL; } \
	}

#define MESSAGE(name, vals) \
	BOOST_PP_IIF( \
		BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE((~)vals), 1), \
		MESSAGE_WITHOUT_INPUTS, \
		MESSAGE_WITH_INPUTS) \
	(name, vals)



#define COMMAND(name, merge, vals) \
	COMMANDDATASTRUCT(name) \
		d##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, vals) ) \
			: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, vals) {} \
		BOOST_PP_SEQ_FOR_EACH_I(B_CONSTMEMBERS, ~, vals) \
	}; \
	COMMANDSTRUCT(name, merge)


// Need different syntax depending on whether there are some input values in the query:

#define QUERY_WITHOUT_INPUTS(name, in_vals, out_vals) \
	QUERYSTRUCT(name) \
		q##name() {} \
		static q##name* CtorType() { return NULL; } \
		BOOST_PP_SEQ_FOR_EACH_I(B_MEMBERS, ~, out_vals) /* other members */ \
	}

#define QUERY_WITH_INPUTS(name, in_vals, out_vals) \
	QUERYSTRUCT(name) \
		q##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, in_vals) ) \
			: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, in_vals) {} \
		static q##name* CtorType( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORTYPES, ~, in_vals) ) { return NULL; } \
		BOOST_PP_SEQ_FOR_EACH_I(B_CONSTMEMBERS, ~, in_vals) \
		BOOST_PP_SEQ_FOR_EACH_I(B_MEMBERS, ~, out_vals) \
	}

#define QUERY(name, in_vals, out_vals) \
	BOOST_PP_IIF( \
		BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE((~)in_vals), 1), \
		QUERY_WITHOUT_INPUTS, \
		QUERY_WITH_INPUTS) \
	(name, in_vals, out_vals)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#else // MESSAGESSETUP_NOTFIRST => clean up the mess

#undef MESSAGESTRUCT
#undef QUERYSTRUCT
#undef COMMANDDATASTRUCT
#undef COMMANDSTRUCT
#undef B_TYPE
#undef B_NAME
#undef B_CONSTRUCTORARGS
#undef B_CONSTRUCTORTYPES
#undef B_CONSTRUCTORINIT
#undef B_CONSTMEMBERS
#undef B_MEMBERS
#undef MESSAGE
#undef COMMAND
#undef QUERY_WITHOUT_INPUTS
#undef QUERY_WITH_INPUTS
#undef QUERY

}

#endif
