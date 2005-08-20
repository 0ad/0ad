#ifndef MESSAGES_H__
#define MESSAGES_H__

#include "MessagePasser.h"

// Structures in this file are passed over the DLL boundary, so some
// carefulness and/or luck is required...

class wxPoint;

namespace AtlasMessage
{

struct Position
{
	Position() : x(0.f), y(0.f), z(0.f) {}
	Position(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
	Position(const wxPoint& pt); // converts screen-space to world-space coords
	float x, y, z; // world coordinates
};


struct IMessage
{
	virtual const char* GetType() const = 0;
	virtual ~IMessage() {}
};

// High-level message types, as a limited form of type-safety to prevent e.g.
// adding input message into the command queue
struct mCommand : public IMessage {};
struct mInput : public IMessage {};

#define COMMAND(t) struct m##t : public mCommand { const char* GetType() const { return #t; }  private: const m##t& operator=(const m##t&); public:
#define INPUT(t)   struct m##t : public mInput   { const char* GetType() const { return #t; }  private: const m##t& operator=(const m##t&); public:

// Messages for doing/undoing/etc world-altering commands
COMMAND(WorldCommand)
mWorldCommand() {}
virtual void* CloneData() const = 0;
virtual bool IsMergeable() const = 0; \
};
COMMAND(DoCommand)
mDoCommand(mWorldCommand* c) : name(c->GetType()), data(c->CloneData()) {}
const std::string name;
const void* data;
};
COMMAND(UndoCommand)};
COMMAND(RedoCommand)};
COMMAND(MergeCommand)};

const bool MERGE = true;
const bool NOMERGE = false;

#define WORLDDATA(t) \
	struct d##t { \
	private: \
		const d##t& operator=(const d##t&); \
	public:

#define WORLDCOMMAND(t, merge) \
	struct m##t : public mWorldCommand, public d##t { \
		m##t(const d##t& d) : d##t(d) {} \
		const char* GetType() const { return #t; } \
		virtual bool IsMergeable() const { return merge; } \
		void* CloneData() const { return new d##t(*this); } \
	private: \
		const m##t& operator=(const m##t&);\
	};

//////////////////////////////////////////////////////////////////////////

COMMAND(CommandString)
mCommandString(const std::string& name_) : name(name_) {}
const std::string name;
};

//////////////////////////////////////////////////////////////////////////

COMMAND(SetContext)
mSetContext(void* /* HDC */ hdc_, void* /* HGLRC */ hglrc_) : hdc(hdc_), hglrc(hglrc_) {};
const void* hdc;
const void* hglrc;
};

COMMAND(ResizeScreen)
mResizeScreen(int width_, int height_) : width(width_), height(height_) {}
const int width, height;
};

//////////////////////////////////////////////////////////////////////////

COMMAND(GenerateMap)
mGenerateMap(int size_) : size(size_) {}
const int size; // size in number of patches
};

//////////////////////////////////////////////////////////////////////////

INPUT(ScrollConstant)
mScrollConstant(int dir_, float speed_) : dir(dir_), speed(speed_) {}
const int dir; // as in enum below
const float speed; // set speed 0.0f to stop scrolling
enum { FORWARDS, BACKWARDS, LEFT, RIGHT };
};

//////////////////////////////////////////////////////////////////////////

WORLDDATA(AlterElevation)
	dAlterElevation(Position pos_, float amount_) : pos(pos_), amount(amount_) {}
	const Position pos;
	const float amount;
};
WORLDCOMMAND(AlterElevation, MERGE)

//////////////////////////////////////////////////////////////////////////


#undef WORLDDATA
#undef WORLDCOMMAND
#undef COMMAND
#undef INPUT

}

#endif // MESSAGES_H__
