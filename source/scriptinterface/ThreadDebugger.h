/* Copyright (C) 2013 Wildfire Games.
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
 
#ifndef INCLUDED_THREADDEBUGGER
#define INCLUDED_THREADDEBUGGER

#include "DebuggingServer.h"
#include "ScriptInterface.h"
#include "scriptinterface/ScriptExtraHeaders.h"

// These Breakpoint classes are not implemented threadsafe. The class using the Breakpoints is responsible to make sure that 
// only one thread accesses the Breakpoint at a time
class CBreakPoint
{
public:
	CBreakPoint() : m_UserLine(0) { }
	
	uint m_UserLine;
	std::string m_Filename;
};

// Only use this with one ScriptInterface/CThreadDebugger!
class CActiveBreakPoint : public CBreakPoint
{
public:
	CActiveBreakPoint()
		: m_ActualLine(m_UserLine)
		, m_Script(NULL)
		, m_Pc(NULL)
		, m_ToRemove(false)
	{ }

	CActiveBreakPoint(CBreakPoint breakPoint)
		: CBreakPoint(breakPoint) // using default copy constructor
		, m_ActualLine(m_UserLine)
		, m_Script(NULL)
		, m_Pc(NULL)
		, m_ToRemove(false)
	{ }
	
	uint m_ActualLine;
	JSScript* m_Script;
	jsbytecode* m_Pc;
	bool m_ToRemove;
};

enum BREAK_SRC { BREAK_SRC_TRAP, BREAK_SRC_INTERRUP, BREAK_SRC_EXCEPTION };

struct ThreadDebugger_impl;

class CThreadDebugger
{
public:
	CThreadDebugger();
	~CThreadDebugger();

	/** @brief Initialize the object (required before using the object!).
	 *
	 * @param	id A unique identifier greater than 0 for the object inside its CDebuggingServer object.
	 * @param 	name A name that will be can be displayed by the UI to identify the thread.
	 * @param	pScriptInterface Pointer to a scriptinterface. All Hooks, breakpoint traps etc. will be registered in this 
	 *          scriptinterface and will be called by the thread this scriptinterface is running in.
	 * @param	pDebuggingServer Pointer to the DebuggingServer object this Object should belong to.
	 *
	 * @return	Return value.
	 */
	void Initialize(uint id, std::string name, ScriptInterface* pScriptInterface, CDebuggingServer* pDebuggingServer);
	
	
	// A bunch of hooks used to get information from spidermonkey.
	// These hooks are used internally only but have to be public because they need to be accessible from the global hook functions.
	// Spidermonkey requires function pointers as hooks, which only works if the functions are global or static (not part of an object).
	// These global functions in ThreadDebugger.cpp are just wrappers for the following member functions.
	
	/** Simply calls BreakHandler with BREAK_SRC_TRAP */
	JSTrapStatus TrapHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, jsval closure);
	/** Hook to capture exceptions and breakpoints in code (throw "Breakpoint";) */
	JSTrapStatus ThrowHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval);
	/** All other hooks call this one if the execution should be paused. It puts the program in a wait-loop and 
	 *  waits for weak-up events (continue, step etc.). */
	JSTrapStatus BreakHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, jsval closure, BREAK_SRC breakSrc);
	JSTrapStatus StepHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void *closure);
	JSTrapStatus StepIntoHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void *closure);
	JSTrapStatus StepOutHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void *closure);
	/** This is an interrup-hook that can be called multiple times per line of code and is used to break into the execution
	  * without previously setting a breakpoint and to break other threads when one thread triggers a breakpoint */
	JSTrapStatus CheckForBreakRequestHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void *closure);
	/** The callback function which gets executed for each new script that gets loaded and each function inside a script.
	 *   We use it for "Execute-Hooks" and "Call-Hooks" in terms of Spidermonkey.
	 *   This hook actually sets the traps (Breakpoints) "on the fly" that have been defined by the user previously.
	 */
	void ExecuteHook(JSContext *cx, const char *filename, unsigned lineno, JSScript *script, JSFunction *fun, void *callerdata);
	/** This hook is used to update the mapping between filename plus line-numbers and jsbytecode pointers */
	void NewScriptHook(JSContext *cx, const char *filename, unsigned lineno, JSScript *script, JSFunction *fun, void *callerdata);
	/** This hook makes sure that invalid mappings between filename plus line-number and jsbytecode points get deleted */
	void DestroyScriptHook(JSContext *cx, JSScript *script);
	

	void ClearTrap(CActiveBreakPoint* activeBreakPoint);
	
	/** @brief Checks if a mapping for the specified filename and line number exists in this CThreadDebugger's context
	 */	
	bool CheckIfMappingPresent(std::string filename, uint line);
	
	/** @brief Checks if a mapping exists for each breakpoint in the list of breakpoints that aren't set yet.
	 *         If there is a mapping, it removes the breakpoint from the list of unset breakpoints (from CDebuggingServer),
	 *         adds it to the list of active breakpoints (CThreadDebugger) and sets a trap.
	 *         Threading: m_Mutex is locked in this call
	 */	
	void SetAllNewTraps();
	
	/** @brief Sets a new trap and stores the information in the CActiveBreakPoint pointer
	 *         Make sure that a mapping exists before calling this function
	 *         Threading: Locking m_Mutex is required by the callee
	 */	
	void SetNewTrap(CActiveBreakPoint* activeBreakPoint, std::string filename, uint line);

	/** @brief Toggle a breakpoint if it's active in this threadDebugger object.
	 *         Threading: Locking m_Mutex is required by the callee
	 *
	 * @param	filename full vfs path to the script filename
	 * @param 	userLine linenumber where the USER set the breakpoint (UserLine)
	 *
	 * @return	true if the breakpoint's state was changed
	 */
	bool ToggleBreakPoint(std::string filename, uint userLine);
	
	
	void GetCallstack(std::stringstream& response);
	void GetStackFrameData(std::stringstream& response, uint nestingLevel, STACK_INFO stackInfoKind);
	
	/** @brief Compares the object's associated scriptinterface with the pointer passed as parameter.
	 * @return true if equal
	 */
	bool CompareScriptInterfacePtr(ScriptInterface* pScriptInterface) const;
	
	// Getter/Setters for members that need to be threadsafe
	std::string GetBreakFileName();
	bool GetIsInBreak();
	uint GetLastBreakLine();
	std::string GetName();
	uint GetID();
	void ContinueExecution();
	void SetNextDbgCmd(DBGCMD dbgCmd);
	DBGCMD GetNextDbgCmd();
	// The callee is responsible for locking m_Mutex
	void AddStackInfoRequest(STACK_INFO requestType, uint nestingLevel, SDL_sem* semaphore);
	
	
private:
	// Getters/Setters for members that need to be threadsafe
	void SetBreakFileName(std::string breakFileName);
	void SetLastBreakLine(uint breakLine);
	void SetIsInBreak(bool isInBreak);
	
	// Other threadsafe functions
	void SaveCallstack();
	
	/// Used only in the scriptinterface's thread.
	void ClearTrapsToRemove();
	bool CurrentFrameIsChildOf(JSStackFrame* pParentFrame);
	void ReturnActiveBreakPoints(jsbytecode* pBytecode);
	void SaveStackFrameData(STACK_INFO stackInfo, uint nestingLevel);
	std::string StringifyCyclicJSON(jsval obj, bool indent);

	std::auto_ptr<ThreadDebugger_impl> m;
};

#endif // INCLUDED_THREADDEBUGGER
