#ifndef _COMMAND_H
#define _COMMAND_H

class CCommand
{
public:
	// virtual destructor
	virtual ~CCommand() {}

	// return the texture name of this command
	virtual const char* GetName() const { return 0; }
	
	// execute this command
	virtual void Execute() = 0;
	
	// can undo command?
	virtual bool IsUndoable() const = 0;
	// undo 
	virtual void Undo() {}
	// redo 
	virtual void Redo() {}
};

#endif
