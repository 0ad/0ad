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

#ifndef INCLUDED_DEBUGGINGSERVER
#define INCLUDED_DEBUGGINGSERVER

#include "third_party/mongoose/mongoose.h"
#include "ScriptInterface.h"

#include "lib/external_libraries/libsdl.h"

class CBreakPoint;
class CThreadDebugger;

enum DBGCMD { DBG_CMD_NONE=0, DBG_CMD_CONTINUE, DBG_CMD_SINGLESTEP, DBG_CMD_STEPINTO, DBG_CMD_STEPOUT };
enum STACK_INFO { STACK_INFO_LOCALS=0, STACK_INFO_THIS, STACK_INFO_GLOBALOBJECT };

class CDebuggingServer
{
public:
	CDebuggingServer();
	~CDebuggingServer();
	
	/** @brief Register a new ScriptInerface for debugging the scripts it executes
	 *
	 * @param name A name for the ScriptInterface (will be sent to the debugging client an probably displayed to the user)
	 * @param pScriptInterface A pointer to the ScriptInterface. This pointer must stay valid until UnRegisterScriptInterface ist called!
	 */
	void RegisterScriptinterface(std::string name, ScriptInterface* pScriptInterface);
	
	/** @brief Unregister a ScriptInerface that was previously registered using RegisterScriptinterface. 
	 *
	 * @param pScriptInterface A pointer to the ScriptInterface
	 */
	void UnRegisterScriptinterface(ScriptInterface* pScriptInterface);

	
	// Mongoose callback when request comes from a client 
	void* MgDebuggingServerCallback(mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info);	
	
	/** @brief Aquire exclusive read and write access to the list of breakpoints.
	 *
	 * @param breakPoints A pointer to the list storing all breakpoints.
	 *
	 * @return  A number you need to pass to ReleaseBreakPointAccess().
	 *
	 * Make sure to call ReleaseBreakPointAccess after you don't need access any longer.
	 * Using this function you get exclusive access and other threads won't be able to access the breakpoints until you call ReleaseBreakPointAccess!
	 */
	double AquireBreakPointAccess(std::list<CBreakPoint>** breakPoints);
	
	/** @brief See AquireBreakPointAccess(). You must not access the pointer returend by AquireBreakPointAccess() any longer after you call this function.
	 * 	
	 * @param breakPointsLockID The number you got when aquiring the access. It's used to make sure that this function never gets
	 *          used by the wrong thread.        
	 */	
	void ReleaseBreakPointAccess(double breakPointsLockID);
	
	
	/// Called from multiple Mongoose threads and multiple ScriptInterface threads
	bool GetBreakRequestedByThread();
	bool GetBreakRequestedByUser();
	// Should other threads be stopped as soon as possible after a breakpoint is triggered in a thread
	bool GetSettingSimultaneousThreadBreak();
	// Should the debugger break on any JS-Exception? If set to false, it will only break when the exceptions text is "Breakpoint".
	bool GetSettingBreakOnException();
	void SetBreakRequestedByThread(bool Enabled);
	void SetBreakRequestedByUser(bool Enabled);
	
private:
	static const char* header400;

	/// Webserver helper function (can be called by multiple mongooser threads)
	bool GetWebArgs(struct mg_connection *conn, const struct mg_request_info* request_info, std::string argName, uint& arg);
	bool GetWebArgs(struct mg_connection *conn, const struct mg_request_info* request_info, std::string argName, std::string& arg);
	
	/// Functions that are made available via http (can be called by multiple mongoose threads)
	void GetThreadDebuggerStatus(std::stringstream& response);
	void ToggleBreakPoint(std::string filename, uint line);
	void GetAllCallstacks(std::stringstream& response);
	void GetStackFrameData(std::stringstream& response, uint nestingLevel, uint threadDebuggerID, STACK_INFO stackInfoKind);
	bool SetNextDbgCmd(uint threadDebuggerID, DBGCMD dbgCmd);
	void SetSettingSimultaneousThreadBreak(bool Enabled);
	void SetSettingBreakOnException(bool Enabled);
	
	/** @brief Returns a list of the full vfs paths to all files with the extension .js found in the vfs root
	 * 
	 *  @param response This will contain the list as JSON array.
	 */
	void EnumVfsJSFiles(std::stringstream& response);
	
	/** @brief Get the content of a .js file loaded into vfs
	 *
	 * @param filename A full vfs path (as returned by EnumVfsJSFiles).
	 * @param response This will contain the contents of the requested file.
	 */
	void GetFile(std::string filename, std::stringstream& response);
	
	/// Shared between multiple mongoose threads
	

	bool m_SettingSimultaneousThreadBreak;
	bool m_SettingBreakOnException;
	
	/// Shared between multiple scriptinterface threads
	uint m_LastThreadDebuggerID;
	
	/// Shared between multiple scriptinerface threads and multiple mongoose threads
	std::list<CThreadDebugger*> m_ThreadDebuggers;
	
	// The CThreadDebuggers will check this value and break the thread if it's true.
	// This only works for JS code, so C++ code will not break until it executes JS-code again.
	bool m_BreakRequestedByThread;
	bool m_BreakRequestedByUser;
	
	// The breakpoint is uniquely identified using filename an line-number.
	// Since the filename is the whole vfs path it should really be unique.
	std::list<CBreakPoint> m_BreakPoints;
	
	/// Used for controlling access to m_BreakPoints
	SDL_sem* m_BreakPointsSem;
	double m_BreakPointsLockID;
	
	/// Mutexes used to ensure thread-safety. Currently we just use one Mutex (m_Mutex) and if we detect possible sources
	/// of deadlocks, we use the second mutex for some members to avoid it.
	CMutex m_Mutex;
	CMutex m_Mutex1;
	
	/// Not important for this class' thread-safety
	void EnableHTTP();
	mg_context* m_MgContext;
};

extern CDebuggingServer* g_DebuggingServer;


#endif // INCLUDED_DEBUGGINGSERVER
