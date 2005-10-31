// Used by Messages.h, so that file stays relatively clean.

#ifndef MESSAGESSETUP_NOTFIRST
#define MESSAGESSETUP_NOTFIRST

#include "MessagePasser.h"

// Structures in this file are passed over the DLL boundary, so some
// carefulness and/or luck is required...

class wxPoint;
class CVector3D;
class CMutex;

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
		// type2: "same as previous" (e.g. for elevation-editing when the mouse hasn't moved)
	};

	// Constructs a position with the meaning "same as previous", which is handled
	// in an unspecified way by various message handlers.
	static Position Unchanged() { Position p; p.type = 2; return p; }

	// Only for use in the game, not the UI.
	// Implementations in Misc.cpp.
	void GetWorldSpace(CVector3D& vec) const;
	void GetWorldSpace(CVector3D& vec, const CVector3D& prev) const;
	void GetScreenSpace(float& x, float& y) const;
};

//////////////////////////////////////////////////////////////////////////

struct IMessage
{
	virtual const char* GetName() const = 0;
	virtual ~IMessage() {}

	enum Type { Message, Query };
	virtual Type GetType() const = 0;
};


#define MESSAGESTRUCT(t) \
	struct m##t : public IMessage { \
		const char* GetName() const { return #t; } \
		Type GetType() const { return Type::Message; } \
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
	const std::string name;
	const void* data;
};
MESSAGESTRUCT(UndoCommand)  };
MESSAGESTRUCT(RedoCommand)  };
MESSAGESTRUCT(MergeCommand) };


struct QueryMessage : public IMessage
{
	Type GetType() const { return Type::Query; }
	void Post() { g_MessagePasser->Query(this); }

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
		const d##t& operator=(const d##t&); \
	public:

#define COMMANDSTRUCT(t, merge) \
	struct m##t : public mWorldCommand, public d##t { \
		m##t(const d##t& d) : d##t(d) {} \
		const char* GetName() const { return #t; } \
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
#define B_CONSTMEMBERS(r, data, n, elem) const B_TYPE(elem) B_NAME(elem);
#define B_MEMBERS(r, data, n, elem) B_TYPE(elem) B_NAME(elem);

#define MESSAGE(name, vals) \
	MESSAGESTRUCT(name) \
		m##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, vals) ) \
			: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, vals) {} \
		BOOST_PP_SEQ_FOR_EACH_I(B_CONSTMEMBERS, ~, vals) \
	};

#define QUERY(name, in_vals, out_vals) \
	QUERYSTRUCT(name) \
		q##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, in_vals) ) \
			: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, in_vals) {} \
		BOOST_PP_SEQ_FOR_EACH_I(B_CONSTMEMBERS, ~, in_vals) \
		BOOST_PP_SEQ_FOR_EACH_I(B_MEMBERS, ~, out_vals) \
	};

#define COMMAND(name, merge, vals) \
	COMMANDDATASTRUCT(name) \
		d##name( BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORARGS, ~, vals) ) \
		: BOOST_PP_SEQ_FOR_EACH_I(B_CONSTRUCTORINIT, ~, vals) {} \
		BOOST_PP_SEQ_FOR_EACH_I(B_CONSTMEMBERS, ~, vals) \
	}; \
	COMMANDSTRUCT(name, merge);

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
#undef B_CONSTRUCTORINIT
#undef B_CONSTMEMBERS
#undef B_MEMBERS
#undef MESSAGE
#undef QUERY
#undef COMMAND

}

#endif
