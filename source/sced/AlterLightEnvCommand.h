#ifndef _ALTERLIGHTENVCOMMAND_H
#define _ALTERLIGHTENVCOMMAND_H

#include <set>
#include "res/res.h"
#include "Command.h"

#include "LightEnv.h"

class CAlterLightEnvCommand : public CCommand
{
public:
	// constructor, destructor
	CAlterLightEnvCommand(const CLightEnv& env);
	~CAlterLightEnvCommand();

	// return the texture name of this command
	const char* GetName() const { return "Alter Lighting Parameters"; }

	// execute this command
	void Execute();
	
	// can undo command?
	bool IsUndoable() const { return true; }
	// undo 
	void Undo();
	// redo 
	void Redo();

private:
	// apply given lighting parameters to global environment
	void ApplyData(const CLightEnv& env);

	// lighting parameters to apply to the environment
	CLightEnv m_LightEnv;
	// old lighting parameters - saved for undo/redo
	CLightEnv m_SavedLightEnv;
};

#endif
