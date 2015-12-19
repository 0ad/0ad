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
 
 // JS debugger temporarily disabled during the SpiderMonkey upgrade (check trac ticket #2348 for details)
#include "precompiled.h" // needs to be here for Windows builds
 /*

#include "ThreadDebugger.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"

#include <map>
#include <queue>

// Hooks

CMutex ThrowHandlerMutex;
static JSTrapStatus ThrowHandler_(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* rval, void* closure)
{
	CScopeLock lock(ThrowHandlerMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) closure;
	return pThreadDebugger->ThrowHandler(cx, script, pc, rval);
}

CMutex TrapHandlerMutex;
static JSTrapStatus TrapHandler_(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* rval, jsval closure) 
{
	CScopeLock lock(TrapHandlerMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) JSVAL_TO_PRIVATE(closure);
	jsval val = JSVAL_NULL;
	return pThreadDebugger->TrapHandler(cx, script, pc, rval, val);
}

CMutex StepHandlerMutex;
JSTrapStatus StepHandler_(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* rval, void* closure)
{
	CScopeLock lock(StepHandlerMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) closure;
	jsval val = JSVAL_VOID;
	return pThreadDebugger->StepHandler(cx, script, pc, rval, &val);
}

CMutex StepIntoHandlerMutex;
JSTrapStatus StepIntoHandler_(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* rval, void* closure)
{
	CScopeLock lock(StepIntoHandlerMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) closure;
	return pThreadDebugger->StepIntoHandler(cx, script, pc, rval, NULL);
}

CMutex NewScriptHookMutex;
void NewScriptHook_(JSContext* cx, const char* filename, unsigned lineno, JSScript* script, JSFunction* fun, void* callerdata)
{
	CScopeLock lock(NewScriptHookMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) callerdata;
	return pThreadDebugger->NewScriptHook(cx, filename, lineno, script, fun, NULL);
}

CMutex DestroyScriptHookMutex;
void DestroyScriptHook_(JSContext* cx, JSScript* script, void* callerdata)
{
	CScopeLock lock(DestroyScriptHookMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) callerdata;
	return pThreadDebugger->DestroyScriptHook(cx, script);
}

CMutex StepOutHandlerMutex;
JSTrapStatus StepOutHandler_(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* rval, void* closure)
{
	CScopeLock lock(StepOutHandlerMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) closure;
	return pThreadDebugger->StepOutHandler(cx, script, pc, rval, NULL);
}

CMutex CheckForBreakRequestHandlerMutex;
JSTrapStatus CheckForBreakRequestHandler_(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* rval, void* closure)
{
	CScopeLock lock(CheckForBreakRequestHandlerMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) closure;
	return pThreadDebugger->CheckForBreakRequestHandler(cx, script, pc, rval, NULL);
}

CMutex CallHookMutex;
static void* CallHook_(JSContext* cx, JSStackFrame* fp, bool before, bool* UNUSED(ok), void* closure)
{
	CScopeLock lock(CallHookMutex);
	CThreadDebugger* pThreadDebugger = (CThreadDebugger*) closure;
	if (before)
	{
		JSScript* script;
		script = JS_GetFrameScript(cx, fp);
		const char* fileName = JS_GetScriptFilename(cx, script);
		uint lineno = JS_GetScriptBaseLineNumber(cx, script);
		JSFunction* fun = JS_GetFrameFunction(cx, fp);
		pThreadDebugger->ExecuteHook(cx, fileName, lineno, script, fun, closure);
	}
	
	return closure;
}

/// ThreadDebugger_impl

struct StackInfoRequest
{
	STACK_INFO requestType;
	uint nestingLevel;
	SDL_sem* semaphore;
};

struct trapLocation
{
	jsbytecode* pBytecode;
	JSScript* pScript;
	uint firstLineInFunction;
	uint lastLineInFunction;
};

struct ThreadDebugger_impl
{
	NONCOPYABLE(ThreadDebugger_impl);
public:

	ThreadDebugger_impl();
	~ThreadDebugger_impl();

	CMutex m_Mutex;
	CMutex m_ActiveBreakpointsMutex;
	CMutex m_NextDbgCmdMutex;
	CMutex m_IsInBreakMutex;

	// This member could actually be used by other threads via CompareScriptInterfacePtr(), but that should be safe
	ScriptInterface* m_pScriptInterface; 
	CDebuggingServer* m_pDebuggingServer;
	// We store the pointer on the heap because the stack frame becomes invalid in certain cases
	// and spidermonkey throws errors if it detects a pointer on the stack.
	// We only use the pointer for comparing it with the current stack pointer and we don't try to access it, so it
	// shouldn't be a problem.
	JSStackFrame** m_pLastBreakFrame;
	uint m_ObjectReferenceID;

	/// shared between multiple mongoose threads and one scriptinterface thread
	std::string m_BreakFileName;
	uint m_LastBreakLine;
	bool m_IsInBreak;
	DBGCMD m_NextDbgCmd;

	std::queue<StackInfoRequest> m_StackInfoRequests;

	std::map<std::string, std::map<uint, trapLocation> > m_LineToPCMap;
	std::list<CActiveBreakPoint*> m_ActiveBreakPoints;
	std::map<STACK_INFO, std::map<uint, std::string> > m_StackFrameData;
	std::string m_Callstack;

	/// shared between multiple mongoose threads (initialization may be an exception)
	std::string m_Name;
	uint m_ID;
};

ThreadDebugger_impl::ThreadDebugger_impl()
	: m_NextDbgCmd(DBG_CMD_NONE)
	, m_pScriptInterface(NULL)
	, m_pDebuggingServer(NULL)
	, m_pLastBreakFrame(new JSStackFrame*)
	, m_IsInBreak(false)
{ }

ThreadDebugger_impl::~ThreadDebugger_impl()
{
	delete m_pLastBreakFrame;
}

/// CThreadDebugger

void CThreadDebugger::ClearTrapsToRemove()
{
	CScopeLock lock(m->m_Mutex);
	std::list<CActiveBreakPoint*>::iterator itr=m->m_ActiveBreakPoints.begin();
	while (itr != m->m_ActiveBreakPoints.end())
	{
		if ((*itr)->m_ToRemove)
		{
			ClearTrap((*itr));
			// Remove the breakpoint
			delete (*itr);
			itr = m->m_ActiveBreakPoints.erase(itr);
		}
		else
			++itr;
	}
}

void CThreadDebugger::ClearTrap(CActiveBreakPoint* activeBreakPoint)
{
	ENSURE(activeBreakPoint->m_Script != NULL && activeBreakPoint->m_Pc != NULL);
	JSTrapHandler prevHandler;
	jsval prevClosure;
	JS_ClearTrap(m->m_pScriptInterface->GetContext(), activeBreakPoint->m_Script, activeBreakPoint->m_Pc, &prevHandler, &prevClosure);
	activeBreakPoint->m_Script = NULL;
	activeBreakPoint->m_Pc = NULL;
}

void CThreadDebugger::SetAllNewTraps()
{
	std::list<CBreakPoint>* pBreakPoints = NULL;
	double breakPointsLockID;
	breakPointsLockID = m->m_pDebuggingServer->AquireBreakPointAccess(&pBreakPoints);
	std::list<CBreakPoint>::iterator itr = pBreakPoints->begin();
	while (itr != pBreakPoints->end())
	{
		if (CheckIfMappingPresent((*itr).m_Filename, (*itr).m_UserLine))
		{
			// We must not set a new trap if we already have set a trap for this line of code.
			// For lines without source code it's possible to have breakpoints set that actually refer to another line 
			// that contains code. This situation is possible if the line containing the sourcecode already has a breakpoint
			// set and the user sets another one by setting a breakpoint on a line directly above without sourcecode.
			bool trapAlreadySet = false;
			{
				CScopeLock lock(m->m_Mutex);
				std::list<CActiveBreakPoint*>::iterator itr1;
				for (itr1 = m->m_ActiveBreakPoints.begin(); itr1 != m->m_ActiveBreakPoints.end(); ++itr1)
				{
					if ((*itr1)->m_ActualLine == (*itr).m_UserLine)
						trapAlreadySet = true;
				}
			}

			if (!trapAlreadySet)
			{
				CActiveBreakPoint* pActiveBreakPoint = new CActiveBreakPoint((*itr));
				SetNewTrap(pActiveBreakPoint, (*itr).m_Filename, (*itr).m_UserLine);
				{
					CScopeLock lock(m->m_Mutex);
					m->m_ActiveBreakPoints.push_back(pActiveBreakPoint);
				}
				itr = pBreakPoints->erase(itr);
				continue;
			}
		}
		++itr;
	}
	m->m_pDebuggingServer->ReleaseBreakPointAccess(breakPointsLockID);
}

bool CThreadDebugger::CheckIfMappingPresent(std::string filename, uint line)
{
	bool isPresent = (m->m_LineToPCMap.end() != m->m_LineToPCMap.find(filename) && m->m_LineToPCMap[filename].end() != m->m_LineToPCMap[filename].find(line));
	return isPresent;
}

void CThreadDebugger::SetNewTrap(CActiveBreakPoint* activeBreakPoint, std::string filename, uint line)
{
	ENSURE(activeBreakPoint->m_Script == NULL); // The trap must not be set already!
	ENSURE(CheckIfMappingPresent(filename, line)); // You have to check if the mapping exists before calling this function!

	jsbytecode* pc = m->m_LineToPCMap[filename][line].pBytecode;
	JSScript* script = m->m_LineToPCMap[filename][line].pScript;
	activeBreakPoint->m_Script = script;
	activeBreakPoint->m_Pc = pc;
	ENSURE(script != NULL && pc != NULL);
	activeBreakPoint->m_ActualLine = JS_PCToLineNumber(m->m_pScriptInterface->GetContext(), script, pc);
	
	JS_SetTrap(m->m_pScriptInterface->GetContext(), script, pc, TrapHandler_, PRIVATE_TO_JSVAL(this));
}


CThreadDebugger::CThreadDebugger() :
	m(new ThreadDebugger_impl())
{
}

CThreadDebugger::~CThreadDebugger()
{
	// Clear all Traps and Breakpoints that are marked for removal
	ClearTrapsToRemove();
	
	// Return all breakpoints to the associated CDebuggingServer
	ReturnActiveBreakPoints(NULL);

	// Remove all the hooks because they store a pointer to this object
	JS_SetExecuteHook(m->m_pScriptInterface->GetJSRuntime(), NULL, NULL);
	JS_SetCallHook(m->m_pScriptInterface->GetJSRuntime(), NULL, NULL);
	JS_SetNewScriptHook(m->m_pScriptInterface->GetJSRuntime(), NULL, NULL);
	JS_SetDestroyScriptHook(m->m_pScriptInterface->GetJSRuntime(), NULL, NULL);
}

void CThreadDebugger::ReturnActiveBreakPoints(jsbytecode* pBytecode)
{
	CScopeLock lock(m->m_ActiveBreakpointsMutex);
	std::list<CActiveBreakPoint*>::iterator itr;
	itr = m->m_ActiveBreakPoints.begin();
	while (itr != m->m_ActiveBreakPoints.end())
	{
		// Breakpoints marked for removal should be deleted instead of returned!
		if ( ((*itr)->m_Pc == pBytecode || pBytecode == NULL) && !(*itr)->m_ToRemove )
		{
			std::list<CBreakPoint>* pBreakPoints;
			double breakPointsLockID = m->m_pDebuggingServer->AquireBreakPointAccess(&pBreakPoints);
			CBreakPoint breakPoint;
			breakPoint.m_UserLine = (*itr)->m_UserLine;
			breakPoint.m_Filename = (*itr)->m_Filename;
			// All active breakpoints should have a trap set
			ClearTrap((*itr));
			pBreakPoints->push_back(breakPoint);
			delete (*itr);
			itr = m->m_ActiveBreakPoints.erase(itr);
			m->m_pDebuggingServer->ReleaseBreakPointAccess(breakPointsLockID);
		}
		else
			++itr;
	}
}

void CThreadDebugger::Initialize(uint id, std::string name, ScriptInterface* pScriptInterface, CDebuggingServer* pDebuggingServer)
{
	ENSURE(id != 0);
	m->m_ID = id;
	m->m_Name = name;
	m->m_pScriptInterface = pScriptInterface;
	m->m_pDebuggingServer = pDebuggingServer;
	JS_SetExecuteHook(m->m_pScriptInterface->GetJSRuntime(), CallHook_, (void*)this);
	JS_SetCallHook(m->m_pScriptInterface->GetJSRuntime(), CallHook_, (void*)this);
	JS_SetNewScriptHook(m->m_pScriptInterface->GetJSRuntime(), NewScriptHook_, (void*)this);
	JS_SetDestroyScriptHook(m->m_pScriptInterface->GetJSRuntime(), DestroyScriptHook_, (void*)this);
	JS_SetThrowHook(m->m_pScriptInterface->GetJSRuntime(), ThrowHandler_, (void*)this);
	
	if (m->m_pDebuggingServer->GetSettingSimultaneousThreadBreak())
	{
		// Setup a handler to check for break-requests from the DebuggingServer regularly
		JS_SetInterrupt(m->m_pScriptInterface->GetJSRuntime(), CheckForBreakRequestHandler_, (void*)this);
	}
}

JSTrapStatus CThreadDebugger::StepHandler(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* rval, void* UNUSED(closure))
{
	// We break in two conditions
	// 1. We are in the same frame but on a different line
	//	  Note: On loops for example, we can go a few lines up again without leaving the current stack frame, so it's not necessarily 
	//          a higher line number.
	// 2. We are in a different Frame and m_pLastBreakFrame is not a parent of the current frame (because we stepped out of the function)
	uint line = JS_PCToLineNumber(cx, script, pc);
	JSStackFrame* iter = NULL;
	JSStackFrame* pStackFrame;
	pStackFrame = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
	uint lastBreakLine = GetLastBreakLine() ;
	jsval val = JSVAL_VOID;
	if ((*m->m_pLastBreakFrame == pStackFrame && lastBreakLine != line) || 
		(*m->m_pLastBreakFrame != pStackFrame && !CurrentFrameIsChildOf(*m->m_pLastBreakFrame)))
		return BreakHandler(cx, script, pc, rval, val, BREAK_SRC_INTERRUP);
	else
		return JSTRAP_CONTINUE;
}

JSTrapStatus CThreadDebugger::StepIntoHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void* UNUSED(closure))
{
	// We break when we are on the same stack frame but not on the same line 
	// or when we are on another stack frame.
	uint line = JS_PCToLineNumber(cx, script, pc);
	JSStackFrame* iter = NULL;
	JSStackFrame* pStackFrame;
	pStackFrame = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
	uint lastBreakLine = GetLastBreakLine();
	
	jsval val = JSVAL_VOID;
	if ((*m->m_pLastBreakFrame == pStackFrame && lastBreakLine != line) || *m->m_pLastBreakFrame != pStackFrame)
		return BreakHandler(cx, script, pc, rval, val, BREAK_SRC_INTERRUP);
	else
		return JSTRAP_CONTINUE;
}

JSTrapStatus CThreadDebugger::StepOutHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void* UNUSED(closure))
{
	// We break when we are in a different Frame and m_pLastBreakFrame is not a parent of the current frame 
	// (because we stepped out of the function)
	JSStackFrame* iter = NULL;
	JSStackFrame* pStackFrame;
	pStackFrame = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
	if (pStackFrame != *m->m_pLastBreakFrame && !CurrentFrameIsChildOf(*m->m_pLastBreakFrame))
	{
		jsval val = JSVAL_VOID;
		return BreakHandler(cx, script, pc, rval, val, BREAK_SRC_INTERRUP);
	}
	else
		return JSTRAP_CONTINUE;
}

bool CThreadDebugger::CurrentFrameIsChildOf(JSStackFrame* pParentFrame)
{
	JSStackFrame* iter = NULL;
	JSStackFrame* fp = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
	// Get the first parent Frame
	fp = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
	while (fp)
	{
		if (fp == pParentFrame) 
			return true;
		fp = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
	}
	return false;
}

JSTrapStatus CThreadDebugger::CheckForBreakRequestHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void* UNUSED(closure))
{
	jsval val = JSVAL_VOID;
	if (m->m_pDebuggingServer->GetBreakRequestedByThread() || m->m_pDebuggingServer->GetBreakRequestedByUser())
		return BreakHandler(cx, script, pc, rval, val, BREAK_SRC_INTERRUP);
	else
		return JSTRAP_CONTINUE;
}

JSTrapStatus CThreadDebugger::TrapHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, jsval UNUSED(closure))
{
	jsval val = JSVAL_NULL;
	return BreakHandler(cx, script, pc, rval, val, BREAK_SRC_TRAP);
}

JSTrapStatus CThreadDebugger::ThrowHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval)
{
	jsval jsexception;
	JS_GetPendingException(cx, &jsexception);
	if (JSVAL_IS_STRING(jsexception))
	{
		std::string str(JS_EncodeString(cx, JSVAL_TO_STRING(jsexception)));
		if (str == "Breakpoint" || m->m_pDebuggingServer->GetSettingBreakOnException())
		{
			if (str == "Breakpoint")
				JS_ClearPendingException(cx);
			jsval val = JSVAL_NULL;
			return BreakHandler(cx, script, pc, rval, val, BREAK_SRC_EXCEPTION);
		}
	}
	return JSTRAP_CONTINUE;
}

JSTrapStatus CThreadDebugger::BreakHandler(JSContext* cx, JSScript* script, jsbytecode* pc, jsval* UNUSED(rval), jsval UNUSED(closure), BREAK_SRC breakSrc) 
{
	uint line = JS_PCToLineNumber(cx, script, pc);
	std::string filename(JS_GetScriptFilename(cx, script));

	SetIsInBreak(true);
	SaveCallstack();
	SetLastBreakLine(line);
	SetBreakFileName(filename);
	*m->m_pLastBreakFrame = NULL;
	
	if (breakSrc == BREAK_SRC_INTERRUP)
	{
		JS_ClearInterrupt(m->m_pScriptInterface->GetJSRuntime(), NULL, NULL);
		JS_SetSingleStepMode(cx, script, false);
	}
	
	if (m->m_pDebuggingServer->GetSettingSimultaneousThreadBreak())
	{
		m->m_pDebuggingServer->SetBreakRequestedByThread(true);
	}
	
	// Wait until the user continues the execution
	while (1)
	{
		DBGCMD nextDbgCmd = GetNextDbgCmd();
		
		while (!m->m_StackInfoRequests.empty())
		{
			StackInfoRequest request = m->m_StackInfoRequests.front();
			SaveStackFrameData(request.requestType, request.nestingLevel);
			SDL_SemPost(request.semaphore);
			m->m_StackInfoRequests.pop();
		}
		
		if (nextDbgCmd == DBG_CMD_NONE)
		{
			// Wait a while before checking for new m_NextDbgCmd again.
			// We don't want this loop to take 100% of a CPU core for each thread that is in break mode.
			// On the other hande we don't want the debugger to become unresponsive.
			SDL_Delay(100);
		}
		else if (nextDbgCmd == DBG_CMD_SINGLESTEP || nextDbgCmd == DBG_CMD_STEPINTO || nextDbgCmd == DBG_CMD_STEPOUT)
		{
			JSStackFrame* iter = NULL;
			*m->m_pLastBreakFrame = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
			
			if (!JS_SetSingleStepMode(cx, script, true))
				LOGERROR("JS_SetSingleStepMode returned false!"); // TODO: When can this happen?
			else
			{
				if (nextDbgCmd == DBG_CMD_SINGLESTEP)
				{
					JS_SetInterrupt(m->m_pScriptInterface->GetJSRuntime(), StepHandler_, this);
					break;
				}
				else if (nextDbgCmd == DBG_CMD_STEPINTO)
				{
					JS_SetInterrupt(m->m_pScriptInterface->GetJSRuntime(), StepIntoHandler_, this);
					break;
				}
				else if (nextDbgCmd == DBG_CMD_STEPOUT)
				{
					JS_SetInterrupt(m->m_pScriptInterface->GetJSRuntime(), StepOutHandler_, this);
					break;
				}
			}
		}
		else if (nextDbgCmd == DBG_CMD_CONTINUE)
		{
			if (!JS_SetSingleStepMode(cx, script, true))
				LOGERROR("JS_SetSingleStepMode returned false!"); // TODO: When can this happen?
			else
			{
				// Setup a handler to check for break-requests from the DebuggingServer regularly
				JS_SetInterrupt(m->m_pScriptInterface->GetJSRuntime(), CheckForBreakRequestHandler_, this);
			}
			break;
		}
		else 
			debug_warn("Invalid DBGCMD found in CThreadDebugger::BreakHandler!");
	}
	ClearTrapsToRemove();
	SetAllNewTraps();
	SetNextDbgCmd(DBG_CMD_NONE);
	SetIsInBreak(false);
	SetBreakFileName(std::string());
	
	// All saved stack data becomes invalid
	{
		CScopeLock lock(m->m_Mutex);
		m->m_StackFrameData.clear();
	}
	
	return JSTRAP_CONTINUE;
}

void CThreadDebugger::NewScriptHook(JSContext* cx, const char* filename, unsigned lineno, JSScript* script, JSFunction* UNUSED(fun), void* UNUSED(callerdata))
{
	uint scriptExtent = JS_GetScriptLineExtent (cx, script);
	std::string stringFileName(filename);
	if (stringFileName.empty())
		return;

	for (uint line = lineno; line < scriptExtent + lineno; ++line) 
	{
		// If we already have a mapping for this line, we check if the current scipt is more deeply nested.
		// If it isn't more deeply nested, we don't overwrite the previous mapping
		// The most deeply nested script is always the one that must be used!
		uint firstLine = 0;
		uint lastLine = 0;
		jsbytecode* oldPC = NULL;
		if (CheckIfMappingPresent(stringFileName, line))
		{
			firstLine = m->m_LineToPCMap[stringFileName][line].firstLineInFunction;
			lastLine = m->m_LineToPCMap[stringFileName][line].lastLineInFunction;
			
			// If an entry nested equally is present too, we must overwrite it.
			// The same script(function) can trigger a NewScriptHook multiple times without DestroyScriptHooks between these 
			// calls. In this case the old script becomes invalid.
			if (lineno < firstLine || scriptExtent + lineno > lastLine)
				continue;
			else
				oldPC = m->m_LineToPCMap[stringFileName][line].pBytecode;
				
		}
		jsbytecode* pc = JS_LineNumberToPC (cx, script, line);
		m->m_LineToPCMap[stringFileName][line].pBytecode = pc;
		m->m_LineToPCMap[stringFileName][line].pScript = script;
		m->m_LineToPCMap[stringFileName][line].firstLineInFunction = lineno;
		m->m_LineToPCMap[stringFileName][line].lastLineInFunction = lineno + scriptExtent;
		
		// If we are replacing a script, the associated traps become invalid
		if (lineno == firstLine && scriptExtent + lineno == lastLine)
		{
			ReturnActiveBreakPoints(oldPC);
			SetAllNewTraps();
		}
	}
}

void CThreadDebugger::DestroyScriptHook(JSContext* cx, JSScript* script)
{
	uint scriptExtent = JS_GetScriptLineExtent (cx, script);
	uint baseLine = JS_GetScriptBaseLineNumber(cx, script);
	
	char* pStr = NULL;
	pStr = (char*)JS_GetScriptFilename(cx, script);
	if (pStr != NULL)
	{
		std::string fileName(pStr);

		for (uint line = baseLine; line < scriptExtent + baseLine; ++line) 
		{
			if (CheckIfMappingPresent(fileName, line))
			{
				if (m->m_LineToPCMap[fileName][line].pScript == script)
				{
					ReturnActiveBreakPoints(m->m_LineToPCMap[fileName][line].pBytecode);
					m->m_LineToPCMap[fileName].erase(line);
					if (m->m_LineToPCMap[fileName].empty())
						m->m_LineToPCMap.erase(fileName);
				}
			}
		}
	}
}

void CThreadDebugger::ExecuteHook(JSContext* UNUSED(cx), const char* UNUSED(filename), unsigned UNUSED(lineno), JSScript* UNUSED(script), JSFunction* UNUSED(fun), void* UNUSED(callerdata))
{
	// Search all breakpoints that have no trap set yet
	{
		PROFILE2("ExecuteHook");
		SetAllNewTraps();
	}
	return;
}

bool CThreadDebugger::ToggleBreakPoint(std::string filename, uint userLine)
{
	CScopeLock lock(m->m_Mutex);
	std::list<CActiveBreakPoint*>::iterator itr;
	for (itr = m->m_ActiveBreakPoints.begin(); itr != m->m_ActiveBreakPoints.end(); ++itr)
	{
		if ((*itr)->m_UserLine == userLine && (*itr)->m_Filename == filename)
		{
			(*itr)->m_ToRemove = !(*itr)->m_ToRemove;
			return true;
		}
	}
	return false;
}

void CThreadDebugger::GetCallstack(std::stringstream& response)
{
	CScopeLock lock(m->m_Mutex);
	response << m->m_Callstack;
}

void CThreadDebugger::SaveCallstack()
{
	ENSURE(GetIsInBreak());
	
	CScopeLock lock(m->m_Mutex);
	
	JSStackFrame *fp;
	JSStackFrame *iter = 0;
	jsint counter = 0;
	
	JSObject* jsArray;
	jsArray = JS_NewArrayObject(m->m_pScriptInterface->GetContext(), 0, 0);
	JSString* functionID;

	fp = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);

	while (fp)
	{
		JSFunction* fun = 0;
		fun = JS_GetFrameFunction(m->m_pScriptInterface->GetContext(), fp);
		if (NULL == fun)
			functionID = JS_NewStringCopyZ(m->m_pScriptInterface->GetContext(), "null");
		else
		{
			functionID = JS_GetFunctionId(fun);
			if (NULL == functionID)
				functionID = JS_NewStringCopyZ(m->m_pScriptInterface->GetContext(), "anonymous");
		}

		bool ret = JS_DefineElement(m->m_pScriptInterface->GetContext(), jsArray, counter, STRING_TO_JSVAL(functionID), NULL, NULL, 0);
		ENSURE(ret);
		fp = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
		counter++;
	}
	
	m->m_Callstack.clear();
	m->m_Callstack = m->m_pScriptInterface->StringifyJSON(OBJECT_TO_JSVAL(jsArray), false).c_str();
}

void CThreadDebugger::GetStackFrameData(std::stringstream& response, uint nestingLevel, STACK_INFO stackInfoKind)
{
	// If the data is not yet cached, request it and wait until it's ready.
	bool dataCached = false;
	{
		CScopeLock lock(m->m_Mutex);
		dataCached = (!m->m_StackFrameData.empty() && m->m_StackFrameData[stackInfoKind].end() != m->m_StackFrameData[stackInfoKind].find(nestingLevel));
	}

	if (!dataCached)
	{
		SDL_sem* semaphore = SDL_CreateSemaphore(0);
		AddStackInfoRequest(stackInfoKind, nestingLevel, semaphore);
		SDL_SemWait(semaphore);
		SDL_DestroySemaphore(semaphore);
	}
	
	CScopeLock lock(m->m_Mutex);
	{
		response.str(std::string());
		response << m->m_StackFrameData[stackInfoKind][nestingLevel];
	}
}

void CThreadDebugger::AddStackInfoRequest(STACK_INFO requestType, uint nestingLevel, SDL_sem* semaphore)
{
	StackInfoRequest request;
	request.requestType = requestType;
	request.semaphore = semaphore;
	request.nestingLevel = nestingLevel;
	m->m_StackInfoRequests.push(request);
}

void CThreadDebugger::SaveStackFrameData(STACK_INFO stackInfo, uint nestingLevel)
{
	ENSURE(GetIsInBreak());
	
	CScopeLock lock(m->m_Mutex);
	JSStackFrame *iter = 0;
	uint counter = 0;
	jsval val;
	
	if (stackInfo == STACK_INFO_GLOBALOBJECT)
	{
		JSObject* obj;
		obj = JS_GetGlobalForScopeChain(m->m_pScriptInterface->GetContext());
		m->m_StackFrameData[stackInfo][nestingLevel] = StringifyCyclicJSON(OBJECT_TO_JSVAL(obj), false);
	}
	else
	{
		JSStackFrame *fp = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
		while (fp)
		{
			if (counter == nestingLevel)
			{
				if (stackInfo == STACK_INFO_LOCALS)
				{
					JSObject* obj;
					obj = JS_GetFrameCallObject(m->m_pScriptInterface->GetContext(), fp);
					//obj = JS_GetFrameScopeChain(m->m_pScriptInterface->GetContext(), fp);
					m->m_StackFrameData[stackInfo][nestingLevel] = StringifyCyclicJSON(OBJECT_TO_JSVAL(obj), false);
				}
				else if (stackInfo == STACK_INFO_THIS)
				{
					if (JS_GetFrameThis(m->m_pScriptInterface->GetContext(), fp, &val))
						m->m_StackFrameData[stackInfo][nestingLevel] = StringifyCyclicJSON(val, false);
					else
						m->m_StackFrameData[stackInfo][nestingLevel] = "";
				}
			}
			
			counter++;
			fp = JS_FrameIterator(m->m_pScriptInterface->GetContext(), &iter);
		}
	}
}
*/

/*
 * TODO: This is very hacky and ugly and should be improved. 
 * It replaces cyclic references with a notification that cyclic references are not supported. 
 * It would be better to create a format that supports cyclic references and allows the UI to display them correctly.
 * Unfortunately this seems to require writing (or embedding) a new serializer to JSON or something similar.
 * 
 * Some things about the implementation which aren't optimal:
 * 1. It uses global variables (they are limited to a namespace though).
 * 2. It has to work around a bug in Spidermonkey.
 * 3. It copies code from CScriptInterface. I did this to separate it cleanly because the debugger should not affect
 *    the rest of the game and because this part of code should be replaced anyway in the future.
 */
 
 /*
namespace CyclicRefWorkaround
{
	std::set<JSObject*> g_ProcessedObjects;
	jsval g_LastKey;
	jsval g_LastValue;
	bool g_RecursionDetectedInPrevReplacer = false;
	uint g_countSameKeys = 0;
	
	struct Stringifier
	{
		static bool callback(const char16_t* buf, uint32 len, void* data)
		{
			utf16string str(buf, buf+len);
			std::wstring strw(str.begin(), str.end());

			Status err; // ignore Unicode errors
			static_cast<Stringifier*>(data)->stream << utf8_from_wstring(strw, &err);
			return true;
		}

		std::stringstream stream;
	};
	
	bool replacer(JSContext* cx, uintN UNUSED(argc), jsval* vp)
	{
		jsval value = JS_ARGV(cx, vp)[1];
		jsval key = JS_ARGV(cx, vp)[0];
		if (g_LastKey == key)
			g_countSameKeys++;
		else
			g_countSameKeys = 0;
		
		if (JSVAL_IS_OBJECT(value))
		{	
			// Work around a spidermonkey bug that causes replacer to be called twice with the same key:
			// https://bugzilla.mozilla.org/show_bug.cgi?id=636079
			// TODO: Remove the workaround as soon as we upgrade to a newer version of Spidermonkey.

			if (g_ProcessedObjects.end() == g_ProcessedObjects.find(JSVAL_TO_OBJECT(value)))
			{
					g_ProcessedObjects.insert(JSVAL_TO_OBJECT(value));
			}
			else if (g_countSameKeys %2 == 0 || g_RecursionDetectedInPrevReplacer)
			{
				g_RecursionDetectedInPrevReplacer = true;
				jsval ret = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Debugger: object removed from output because of cyclic reference."));
				JS_SET_RVAL(cx, vp, ret);
				g_LastKey = key;
				g_LastValue = value;
				return true;
			}
		}
		g_LastKey = key;
		g_LastValue = value;
		g_RecursionDetectedInPrevReplacer = false;
		JS_SET_RVAL(cx, vp, JS_ARGV(cx, vp)[1]);
		return true;
	}
}

std::string CThreadDebugger::StringifyCyclicJSON(jsval obj, bool indent)
{
	CyclicRefWorkaround::Stringifier str;
	CyclicRefWorkaround::g_ProcessedObjects.clear();
	CyclicRefWorkaround::g_LastKey = JSVAL_VOID;
	
	JSObject* pGlob = JSVAL_TO_OBJECT(m->m_pScriptInterface->GetGlobalObject());
	JSFunction* fun = JS_DefineFunction(m->m_pScriptInterface->GetContext(), pGlob, "replacer", CyclicRefWorkaround::replacer, 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JSObject* replacer = JS_GetFunctionObject(fun);
	if (!JS_Stringify(m->m_pScriptInterface->GetContext(), &obj, replacer, indent ? INT_TO_JSVAL(2) : JSVAL_VOID, &CyclicRefWorkaround::Stringifier::callback, &str))
	{
		LOGERROR("StringifyJSON failed");
		jsval exec;
		jsval execString;
		if (JS_GetPendingException(m->m_pScriptInterface->GetContext(), &exec))
		{
			if (JSVAL_IS_OBJECT(exec))
			{
				JS_GetProperty(m->m_pScriptInterface->GetContext(), JSVAL_TO_OBJECT(exec), "message", &execString);
			
				if (JSVAL_IS_STRING(execString))
				{
					std::string strExec = JS_EncodeString(m->m_pScriptInterface->GetContext(), JSVAL_TO_STRING(execString));
					LOGERROR("Error: %s", strExec.c_str());
				}
			}
			
		}			
		JS_ClearPendingException(m->m_pScriptInterface->GetContext());
		return std::string();
	}

	return str.stream.str();
}


bool CThreadDebugger::CompareScriptInterfacePtr(ScriptInterface* pScriptInterface) const
{
	return (pScriptInterface == m->m_pScriptInterface);
}


std::string CThreadDebugger::GetBreakFileName()
{
	CScopeLock lock(m->m_Mutex);
	return m->m_BreakFileName;
}

void CThreadDebugger::SetBreakFileName(std::string breakFileName)
{
	CScopeLock lock(m->m_Mutex);
	m->m_BreakFileName = breakFileName;
}

uint CThreadDebugger::GetLastBreakLine()
{
	CScopeLock lock(m->m_Mutex);
	return m->m_LastBreakLine;
}

void CThreadDebugger::SetLastBreakLine(uint breakLine)
{
	CScopeLock lock(m->m_Mutex);
	m->m_LastBreakLine = breakLine;
}

bool CThreadDebugger::GetIsInBreak()
{
	CScopeLock lock(m->m_IsInBreakMutex);
	return m->m_IsInBreak;
}

void CThreadDebugger::SetIsInBreak(bool isInBreak)
{
	CScopeLock lock(m->m_IsInBreakMutex);
	m->m_IsInBreak = isInBreak;
}

void CThreadDebugger::SetNextDbgCmd(DBGCMD dbgCmd)
{
	CScopeLock lock(m->m_NextDbgCmdMutex);
	m->m_NextDbgCmd = dbgCmd;
}

DBGCMD CThreadDebugger::GetNextDbgCmd()
{
	CScopeLock lock(m->m_NextDbgCmdMutex);
	return m->m_NextDbgCmd;
}

std::string CThreadDebugger::GetName()
{
	CScopeLock lock(m->m_Mutex);
	return m->m_Name;
}

uint CThreadDebugger::GetID()
{
	CScopeLock lock(m->m_Mutex);
	return m->m_ID;
}
*/
