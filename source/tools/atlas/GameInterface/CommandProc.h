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

#ifndef INCLUDED_COMMANDPROC
#define INCLUDED_COMMANDPROC

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

/*
We want the command handler implementations to be as pretty as possible - so let people write

	BEGIN_COMMAND(CommandName)
	{
		int member;
		cCommandName() { ... }  // } both are optional; neither may
		~cCommandName() { ... } // }   make use of this->msg

		void Do() { ... interact with this->msg ... }
		void Undo() { ... }
		void Redo() { ... }

		void MergeIntoPrevious(cCommandName* prev) { ... } // iff this command is a mergeable one
	};
	END_COMMAND(CommandName)

which looks almost exactly like a class definition, and is about the simplest
system I can find.


The following macros convert that into:

	class cCommandName_base : public Command
	{
	protected:
		// Storage for data passed into this command
		dCommandName* msg;
	public:
		// Ensure msg is initialised to something 'safe'
		cCommandName_base : msg(NULL) {}

		// MergeIntoPrevious should be overridden by mergeable commands, and implemented
		// to update 'prev' to include the effects of 'this'. (A subclass overriding
		// any function named 'MergeIntoPrevious' will hide this function, even if
		// the types differ.)
		void MergeIntoPrevious(void*) { ...error - needs to be overridden... }
	};

	// Use 'struct' for automatic publicness - we want users to write as little as possible
	struct cCommandName : public cCommandName_base
	{
		// (user's command code - mostly overriding virtual methods from Command)
	}

	// Subclass the command to add things which require knowledge of the
	// complete class definition
	class cCommandName_sub : public cCommandName
	{
	public:
		cCommandName_sub(dCommandName *data) { ...set msg... }
		~cCommandName_sub() { ...clear msg... }

		// Implement the relevant virtual methods from the Command base class,
		// with automatic casting to the correct types and stuff
		virtual void Merge(Command* prev) { ...call MergeIntoPrevious... }
		virtual const char* GetType() const { return "CommandName"; }

		// Factory method for creating an instance of this class, casting the
		// data pointer into the right form (to avoid forcing the generic
		// command-handling support code to worry about the types)
		static Command* Create(const void* data) { ...return new cCommandName_sub... }
	};

	// Register.cpp wants to get that Create method, but it doesn't want to
	// load all the class definitions; so define a simple method that just
	// returns the address of Create
	cmdHandler cCommandName_create()
	{
		return &cCommandName_sub::Create;
	}

// (TODO: make sure this stays in sync with the code below)

*/

#define BEGIN_COMMAND(t) \
	class c##t##_base : public Command \
	{ \
	protected: \
		d##t* msg; \
	public: \
		c##t##_base() : msg(NULL) {} \
		void MergeIntoPrevious(void*) { debug_warn(L"MergeIntoPrevious unimplemented in command " WIDEN(#t)); } \
	}; \
	struct c##t : public c##t##_base

#define END_COMMAND(t) \
	class c##t##_sub : public c##t \
	{ \
	public: \
		c##t##_sub(d##t* data) \
		{ \
			msg = data; \
		} \
		~c##t##_sub() \
		{ \
			/* (msg was allocated in mDoCommand(), using SHAREABLE_NEW) */ \
			AtlasMessage::ShareableDelete(msg); \
			msg = NULL; \
		} \
		virtual void Merge(Command* prev) { MergeIntoPrevious(static_cast<c##t##_sub*>(prev)); } \
		virtual const char* GetType() const { return #t; } \
		static Command* Create(const void* data) \
		{ \
			return new c##t##_sub (reinterpret_cast<d##t*>(const_cast<void*>(data))); \
		} \
	}; \
	cmdHandler c##t##_create() \
	{ \
		return &c##t##_sub ::Create; \
	}


}

#endif // INCLUDED_COMMANDPROC
