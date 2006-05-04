#include <string>
#include <list>
#include <map>

#include "SharedMemory.h"

namespace AtlasMessage
{

struct Command
{
	virtual ~Command() {}
	virtual void Do() = 0;
	virtual void Undo() = 0;
	virtual void Redo() = 0;
	virtual void Merge(Command* prev) = 0;
	virtual const char* GetType() const = 0;
};

class CommandProc
{
public:
	CommandProc();
	~CommandProc();

	// Should be called before shutting down, so it can free
	// references to entities/etc that are stored in commands
	void Destroy();

	void Submit(Command* cmd);

	void Undo();
	void Redo();
	void Merge();

private:
	std::list<Command*> m_Commands;
	typedef std::list<Command*>::iterator cmdIt;
	// The 'current' command is the latest one which has been executed
	// (ignoring any that have been undone)
	cmdIt m_CurrentCommand;
};

typedef Command* (*cmdHandler)(const void*);
typedef std::map<std::string, cmdHandler> cmdHandlers;
extern cmdHandlers& GetCmdHandlers();

CommandProc& GetCommandProc();

struct DataCommand : public Command // so commands can optionally override (De|Con)struct
{
	void Destruct() {};
	void Construct() {};
	// MergeWithSelf should be overridden by commands, and implemented
	// to update 'prev' to include the effects of 'this'
	void MergeWithSelf(void*) { debug_warn("MergeWithSelf unimplemented in some command"); }
};

#define BEGIN_COMMAND(t) \
	class c##t : public DataCommand \
	{ \
		d##t* msg; \
	public: \
		c##t(d##t* data) : msg(data) { Construct(); } \
		~c##t() { Destruct(); AtlasMessage::ShareableDelete(msg); /* msg was allocated by mDoCommand() */ } \
		static Command* Create(const void* data) { return new c##t ((d##t*)data); } \
		virtual void Merge(Command* prev) { MergeWithSelf((c##t*)prev); } \
		virtual const char* GetType() const { return #t; }

#define END_COMMAND(t) \
	}; \
	namespace register_command_##t { \
		struct init { init() { \
			bool notAlreadyRegisted = GetCmdHandlers().insert(std::pair<std::string, cmdHandler>("c"#t, &c##t ::Create)).second; \
			debug_assert(notAlreadyRegisted); \
		} } init; \
	};

}
