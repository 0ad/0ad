// Used by Messages.h, so that file stays relatively clean.

#ifndef MESSAGESSETUP_NOTFIRST
#define MESSAGESSETUP_NOTFIRST

#include "MessagePasser.h"

// Structures in this file are passed over the DLL boundary, so some
// carefulness and/or luck is required...

class wxPoint;
class CVector3D;

namespace AtlasMessage
{

//////////////////////////////////////////////////////////////////////////

struct Position
{
	Position() : type(0) { type0.x = type0.y = type0.z = 0.f;  }
	Position(float x_, float y_, float z_) : type(0) { type0.x = x_; type0.y = y_; type0.z = z_; }
	Position(const wxPoint& pt); // (implementation in ScenarioEditor.cpp)

	int type;
	union {
		struct { float x, y, z; } type0; // world-space coordinates
		struct { int x, y; } type1; // screen-space coordinates, to be projected onto terrain
	};

	// Only for use in the game, not the UI.
	void GetWorldSpace(CVector3D& vec) const; // (implementation in Misc.cpp)
	void GetScreenSpace(float& x, float& y) const; // (implementation in Misc.cpp)
};

//////////////////////////////////////////////////////////////////////////

struct IMessage
{
	virtual const char* GetType() const = 0;
	virtual ~IMessage() {}
};

// High-level message types, as a limited form of type-safety to prevent e.g.
// adding input message into the command queue
struct mCommand : public IMessage {};
struct mInput : public IMessage {};


#define MESSAGESTRUCT(t, b) struct m##t : public m##b { const char* GetType() const { return #t; }  private: const m##t& operator=(const m##t&); public:
#define COMMANDSTRUCT(t) MESSAGESTRUCT(t, Command)
#define   INPUTSTRUCT(t) MESSAGESTRUCT(t, Input)

// Messages for doing/undoing/etc world-altering commands
COMMANDSTRUCT(WorldCommand)
	mWorldCommand() {}
	virtual void* CloneData() const = 0;
	virtual bool IsMergeable() const = 0;
};
COMMANDSTRUCT(DoCommand)
	mDoCommand(mWorldCommand* c) : name(c->GetType()), data(c->CloneData()) {}
	const std::string name;
	const void* data;
};
COMMANDSTRUCT(UndoCommand)  };
COMMANDSTRUCT(RedoCommand)  };
COMMANDSTRUCT(MergeCommand) };

const bool MERGE = true;
const bool NOMERGE = false;


#define WORLDDATASTRUCT(t) \
	struct d##t { \
	private: \
		const d##t& operator=(const d##t&); \
	public:

#define WORLDCOMMANDSTRUCT(t, merge) \
	struct m##t : public mWorldCommand, public d##t { \
		m##t(const d##t& d) : d##t(d) {} \
		const char* GetType() const { return #t; } \
		virtual bool IsMergeable() const { return merge; } \
		void* CloneData() const { return new d##t(*this); } \
	private: \
		const m##t& operator=(const m##t&);\
	};



#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#define B_TYPE(elem) BOOST_PP_TUPLE_ELEM(2, 0, elem)
#define B_NAME(elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)
#define B_CONSTRUCTORARGS(r, data, n, elem) BOOST_PP_COMMA_IF(n) B_TYPE(elem) BOOST_PP_CAT(B_NAME(elem),_)
#define B_CONSTRUCTORINIT(r, data, n, elem) BOOST_PP_COMMA_IF(n) B_NAME(elem)(BOOST_PP_CAT(B_NAME(elem),_))
#define B_MEMBERS(r, data, n, elem) const B_TYPE(elem) B_NAME(elem);

#define B_MESSAGE(name, vals, base) \
	MESSAGESTRUCT(name, base) \
		m##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, vals) ) \
		: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, vals) {} \
		BOOST_PP_SEQ_FOR_EACH_I(B_MEMBERS, ~, vals) \
	};

#define COMMAND(name, vals) B_MESSAGE(name, vals, Command)
#define   INPUT(name, vals) B_MESSAGE(name, vals, Input)

#define WORLDCOMMAND(name, merge, vals) \
	WORLDDATASTRUCT(name) \
		d##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, vals) ) \
		: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, vals) {} \
		BOOST_PP_SEQ_FOR_EACH_I(B_MEMBERS, ~, vals) \
	}; \
	WORLDCOMMANDSTRUCT(name, merge);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#else // MESSAGESSETUP_NOTFIRST => clean up the mess

#undef MESSAGESTRUCT
#undef COMMANDSTRUCT
#undef INPUTSTRUCT
#undef WORLDDATASTRUCT
#undef WORLDCOMMANDSTRUCT
#undef B_TYPE
#undef B_NAME
#undef B_CONSTRUCTORARGS
#undef B_CONSTRUCTORINIT
#undef B_MEMBERS
#undef B_MESSAGE
#undef COMMAND
#undef INPUT
#undef WORLDCOMMAND

}

#endif