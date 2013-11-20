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

#include "precompiled.h"

#include "DebuggingServer.h"
#include "ThreadDebugger.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"

CDebuggingServer* g_DebuggingServer = NULL;

const char* CDebuggingServer::header400 =
		"HTTP/1.1 400 Bad Request\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n"
		"Invalid request";

void CDebuggingServer::GetAllCallstacks(std::stringstream& response)
{
	CScopeLock lock(m_Mutex);
	response.str(std::string());
	std::stringstream stream;
	uint nbrCallstacksWritten = 0;
	std::list<CThreadDebugger*>::iterator itr;
	if (!m_ThreadDebuggers.empty())
	{
		response << "[";
		for (itr = m_ThreadDebuggers.begin(); itr != m_ThreadDebuggers.end(); ++itr)
		{
			if ((*itr)->GetIsInBreak())
			{
				stream.str(std::string());
				(*itr)->GetCallstack(stream);
				if ((int)stream.tellp() != 0)
				{
					if (nbrCallstacksWritten != 0)
						response << ",";
					response << "{" << "\"ThreadDebuggerID\" : " << (*itr)->GetID() << ", \"CallStack\" : " << stream.str() << "}";
					nbrCallstacksWritten++;
				}

			}
		}
		response << "]";
	}
}

void CDebuggingServer::GetStackFrameData(std::stringstream& response, uint nestingLevel, uint threadDebuggerID, STACK_INFO stackInfoKind)
{
	CScopeLock lock(m_Mutex);
	response.str(std::string());
	std::stringstream stream;
	std::list<CThreadDebugger*>::iterator itr;
	for (itr = m_ThreadDebuggers.begin(); itr != m_ThreadDebuggers.end(); ++itr)
	{
		if ((*itr)->GetID() == threadDebuggerID && (*itr)->GetIsInBreak())
		{
			(*itr)->GetStackFrameData(stream, nestingLevel, stackInfoKind);
			if ((int)stream.tellp() != 0)
			{
				response << stream.str();
			}
		}
	}
}


CDebuggingServer::CDebuggingServer() :
	m_MgContext(NULL)
{
	m_BreakPointsSem = SDL_CreateSemaphore(0);
	ENSURE(m_BreakPointsSem);
	SDL_SemPost(m_BreakPointsSem);
	m_LastThreadDebuggerID = 0; // Next will be 1, 0 is reserved
	m_BreakRequestedByThread = false;
	m_BreakRequestedByUser = false;
	m_SettingSimultaneousThreadBreak = true;
	m_SettingBreakOnException = true;

	EnableHTTP();
	LOGWARNING(L"Javascript debugging webserver enabled.");
}

CDebuggingServer::~CDebuggingServer()
{
	SDL_DestroySemaphore(m_BreakPointsSem);
	if (m_MgContext)
	{
		mg_stop(m_MgContext);
		m_MgContext = NULL;
	}
}

bool CDebuggingServer::SetNextDbgCmd(uint threadDebuggerID, DBGCMD dbgCmd)
{
	CScopeLock lock(m_Mutex);
	std::list<CThreadDebugger*>::iterator itr;
	for (itr = m_ThreadDebuggers.begin(); itr != m_ThreadDebuggers.end(); ++itr)
	{
		if ((*itr)->GetID() == threadDebuggerID || threadDebuggerID == 0)
		{
			if (DBG_CMD_NONE == (*itr)->GetNextDbgCmd() && (*itr)->GetIsInBreak())
			{
				SetBreakRequestedByThread(false);
				SetBreakRequestedByUser(false);
				(*itr)->SetNextDbgCmd(dbgCmd);
			}
		}
	}
	return true;
}

void CDebuggingServer::SetBreakRequestedByThread(bool Enabled)
{
	CScopeLock lock(m_Mutex1);
	m_BreakRequestedByThread = Enabled;
}

bool CDebuggingServer::GetBreakRequestedByThread()
{
	CScopeLock lock(m_Mutex1);
	return m_BreakRequestedByThread;
}

void CDebuggingServer::SetBreakRequestedByUser(bool Enabled)
{
	CScopeLock lock(m_Mutex1);
	m_BreakRequestedByUser = Enabled;
}

bool CDebuggingServer::GetBreakRequestedByUser()
{
	CScopeLock lock(m_Mutex1);
	return m_BreakRequestedByUser;
}

void CDebuggingServer::SetSettingBreakOnException(bool Enabled)
{
	CScopeLock lock(m_Mutex);
	m_SettingBreakOnException = Enabled;
}

void CDebuggingServer::SetSettingSimultaneousThreadBreak(bool Enabled)
{
	CScopeLock lock(m_Mutex1);
	m_SettingSimultaneousThreadBreak = Enabled;
}

bool CDebuggingServer::GetSettingBreakOnException()
{
	CScopeLock lock(m_Mutex);
	return m_SettingBreakOnException;
}

bool CDebuggingServer::GetSettingSimultaneousThreadBreak()
{
	CScopeLock lock(m_Mutex1);
	return m_SettingSimultaneousThreadBreak;
}


static Status AddFileResponse(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::string>& templates = *(std::vector<std::string>*)cbData;
	std::wstring str(pathname.string());
	templates.push_back(std::string(str.begin(), str.end()));
	return INFO::OK;
}

void CDebuggingServer::EnumVfsJSFiles(std::stringstream& response)
{
	VfsPath path = L"";
	response.str(std::string());
	
	std::vector<std::string> templates;
	vfs::ForEachFile(g_VFS, "", AddFileResponse, (uintptr_t)&templates, L"*.js", vfs::DIR_RECURSIVE);

	std::vector<std::string>::iterator itr;
	response << "[";
	for (itr = templates.begin(); itr != templates.end(); ++itr)
	{
		if (itr != templates.begin())
			response << ",";
		response << "\"" << *itr << "\"";
	}
	response << "]";
}

void CDebuggingServer::GetFile(std::string filename, std::stringstream& response)
{
	CVFSFile file;
	if (file.Load(g_VFS, filename) != PSRETURN_OK)
	{
		response << "Failed to load the file contents";
		return;
	}

	std::string code = file.DecodeUTF8(); // assume it's UTF-8
	response << code;
}


static void* MgDebuggingServerCallback_(mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info)
{
	CDebuggingServer* debuggingServer = (CDebuggingServer*)request_info->user_data;
	ENSURE(debuggingServer);
	return debuggingServer->MgDebuggingServerCallback(event, conn, request_info);
}

void* CDebuggingServer::MgDebuggingServerCallback(mg_event event, struct mg_connection *conn, const struct mg_request_info *request_info)
{
	void* handled = (void*)""; // arbitrary non-NULL pointer to indicate successful handling

	const char* header200 =
		"HTTP/1.1 200 OK\r\n"
		"Access-Control-Allow-Origin: *\r\n" // TODO: not great for security
		"Content-Type: text/plain; charset=utf-8\r\n\r\n";

	const char* header404 =
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n\r\n"
		"Unrecognised URI";

	switch (event)
	{
	case MG_NEW_REQUEST:
	{
		std::stringstream stream;
		std::string uri = request_info->uri;
		
		if (uri == "/GetThreadDebuggerStatus")
		{
			GetThreadDebuggerStatus(stream);
		}
		else if (uri == "/EnumVfsJSFiles")
		{
			EnumVfsJSFiles(stream);
		}
		else if (uri == "/GetAllCallstacks")
		{
			GetAllCallstacks(stream);
		}
		else if (uri == "/Continue")
		{
			uint threadDebuggerID;
			if (!GetWebArgs(conn, request_info, "threadDebuggerID", threadDebuggerID))
				return handled;
			// TODO: handle the return value
			SetNextDbgCmd(threadDebuggerID, DBG_CMD_CONTINUE);	
		}
		else if (uri == "/Break")
		{
			SetBreakRequestedByUser(true);
		}
		else if (uri == "/SetSettingSimultaneousThreadBreak")
		{
			std::string strEnabled;
			bool bEnabled = false;
			if (!GetWebArgs(conn, request_info, "enabled", strEnabled))
				return handled;
			// TODO: handle the return value
			if (strEnabled == "true")
				bEnabled = true;
			else if (strEnabled == "false")
				bEnabled = false;
			else
				return handled; // TODO: return an error state
			SetSettingSimultaneousThreadBreak(bEnabled);
		}
		else if (uri == "/GetSettingSimultaneousThreadBreak")
		{
			stream << "{ \"Enabled\" : " << (GetSettingSimultaneousThreadBreak() ? "true" : "false") << " } ";
		}
		else if (uri == "/SetSettingBreakOnException")
		{
			std::string strEnabled;
			bool bEnabled = false;
			if (!GetWebArgs(conn, request_info, "enabled", strEnabled))
				return handled;
			// TODO: handle the return value
			if (strEnabled == "true")
				bEnabled = true;
			else if (strEnabled == "false")
				bEnabled = false;
			else
				return handled; // TODO: return an error state
			SetSettingBreakOnException(bEnabled);
		}
		else if (uri == "/GetSettingBreakOnException")
		{
			stream << "{ \"Enabled\" : " << (GetSettingBreakOnException() ? "true" : "false") << " } ";
		}
		else if (uri == "/Step")
		{
			uint threadDebuggerID;
			if (!GetWebArgs(conn, request_info, "threadDebuggerID", threadDebuggerID))
				return handled;
			// TODO: handle the return value
			SetNextDbgCmd(threadDebuggerID, DBG_CMD_SINGLESTEP);
		}
		else if (uri == "/StepInto")
		{
			uint threadDebuggerID;
			if (!GetWebArgs(conn, request_info, "threadDebuggerID", threadDebuggerID))
				return handled;
			// TODO: handle the return value
			SetNextDbgCmd(threadDebuggerID, DBG_CMD_STEPINTO);
		}
		else if (uri == "/StepOut")
		{
			uint threadDebuggerID;
			if (!GetWebArgs(conn, request_info, "threadDebuggerID", threadDebuggerID))
				return handled;
			// TODO: handle the return value
			SetNextDbgCmd(threadDebuggerID, DBG_CMD_STEPOUT);
		}
		else if (uri == "/GetStackFrame")
		{
			uint nestingLevel;
			uint threadDebuggerID;
			if (!GetWebArgs(conn, request_info, "nestingLevel", nestingLevel) || 
				!GetWebArgs(conn, request_info, "threadDebuggerID", threadDebuggerID))
			{
				return handled;
			}
			GetStackFrameData(stream, nestingLevel, threadDebuggerID, STACK_INFO_LOCALS);
		}
		else if (uri == "/GetStackFrameThis")
		{
			uint nestingLevel;
			uint threadDebuggerID;
			if (!GetWebArgs(conn, request_info, "nestingLevel", nestingLevel) || 
				!GetWebArgs(conn, request_info, "threadDebuggerID", threadDebuggerID))
			{
				return handled;
			}
			GetStackFrameData(stream, nestingLevel, threadDebuggerID, STACK_INFO_THIS);
		}
		else if (uri == "/GetCurrentGlobalObject")
		{
			uint threadDebuggerID;
			if (!GetWebArgs(conn, request_info, "threadDebuggerID", threadDebuggerID))
			{
				return handled;
			}
			GetStackFrameData(stream, 0, threadDebuggerID, STACK_INFO_GLOBALOBJECT);
		}
		else if (uri == "/ToggleBreakpoint")
		{
			std::string filename;
			uint line;
			if (!GetWebArgs(conn, request_info, "filename", filename) ||
				!GetWebArgs(conn, request_info, "line", line))
			{
				return handled;
			}
			ToggleBreakPoint(filename, line);
		}
		else if (uri == "/GetFile")
		{
			std::string filename;
			if (!GetWebArgs(conn, request_info, "filename", filename))
				return handled;
			GetFile(filename, stream);
		}
		else
		{
			mg_printf(conn, "%s", header404);
			return handled;
		}

		mg_printf(conn, "%s", header200);
		std::string str = stream.str();
		mg_write(conn, str.c_str(), str.length());
		return handled;
	}

	case MG_HTTP_ERROR:
		return NULL;

	case MG_EVENT_LOG:
		// Called by Mongoose's cry()
		LOGERROR(L"Mongoose error: %hs", request_info->log_message);
		return NULL;

	case MG_INIT_SSL:
		return NULL;

	default:
		debug_warn(L"Invalid Mongoose event type");
		return NULL;
	}
};

void CDebuggingServer::EnableHTTP()
{
	// Ignore multiple enablings
	if (m_MgContext)
		return;

	const char *options[] = {
		"listening_ports", "127.0.0.1:9000", // bind to localhost for security
		"num_threads", "6", // enough for the browser's parallel connection limit
		NULL
	};
	m_MgContext = mg_start(MgDebuggingServerCallback_, this, options);
	ENSURE(m_MgContext);
}

bool CDebuggingServer::GetWebArgs(struct mg_connection *conn, const struct mg_request_info* request_info, std::string argName, uint& arg)
{
	if (!request_info->query_string)
	{
		mg_printf(conn, "%s (no query string)", header400);
		return false;
	}
			
	char buf[256];
			
	int len = mg_get_var(request_info->query_string, strlen(request_info->query_string), argName.c_str(), buf, ARRAY_SIZE(buf));
	if (len < 0)
	{
		mg_printf(conn, "%s (no '%s')", header400, argName.c_str());
		return false;
	}
	arg = atoi(buf);
	return true;
}

bool CDebuggingServer::GetWebArgs(struct mg_connection *conn, const struct mg_request_info* request_info, std::string argName, std::string& arg)
{
	if (!request_info->query_string)
	{
		mg_printf(conn, "%s (no query string)", header400);
		return false;
	}
			
	char buf[256];
	int len = mg_get_var(request_info->query_string, strlen(request_info->query_string), argName.c_str(), buf, ARRAY_SIZE(buf));
	if (len < 0)
	{
		mg_printf(conn, "%s (no '%s')", header400, argName.c_str());
		return false;
	}
	arg = buf;
	return true;
}

void CDebuggingServer::RegisterScriptinterface(std::string name, ScriptInterface* pScriptInterface)
{
	CScopeLock lock(m_Mutex);
	CThreadDebugger* pThreadDebugger = new CThreadDebugger;
	// ThreadID 0 is reserved
	pThreadDebugger->Initialize(++m_LastThreadDebuggerID, name, pScriptInterface, this);
	m_ThreadDebuggers.push_back(pThreadDebugger);
}

void CDebuggingServer::UnRegisterScriptinterface(ScriptInterface* pScriptInterface)
{
	CScopeLock lock(m_Mutex);
	std::list<CThreadDebugger*>::iterator itr;
	for (itr = m_ThreadDebuggers.begin(); itr != m_ThreadDebuggers.end(); ++itr)
	{
		if ((*itr)->CompareScriptInterfacePtr(pScriptInterface))
		{
			delete (*itr);
			m_ThreadDebuggers.erase(itr);
			break;
		}
	}
}


void CDebuggingServer::GetThreadDebuggerStatus(std::stringstream& response)
{
	CScopeLock lock(m_Mutex);
	response.str(std::string());
	std::list<CThreadDebugger*>::iterator itr;

	response << "[";
	for (itr = m_ThreadDebuggers.begin(); itr != m_ThreadDebuggers.end(); ++itr)
	{
		if (itr == m_ThreadDebuggers.begin())
			response << "{ ";
		else
			response << ",{ ";
		
		response << "\"ThreadDebuggerID\" : " << (*itr)->GetID() << ",";
		response << "\"ScriptInterfaceName\" : \"" << (*itr)->GetName() << "\",";
		response << "\"ThreadInBreak\" : " << ((*itr)->GetIsInBreak() ? "true" : "false") << ",";
		response << "\"BreakFileName\" : \"" << (*itr)->GetBreakFileName() << "\",";
		response << "\"BreakLine\" : " << (*itr)->GetLastBreakLine();
		response << " }";
	}
	response << "]";
}

void CDebuggingServer::ToggleBreakPoint(std::string filename, uint line)
{
	// First, pass the message to all associated CThreadDebugger objects and check if one returns true (handled);
	{
		CScopeLock lock(m_Mutex);
		std::list<CThreadDebugger*>::iterator itr;
		for (itr = m_ThreadDebuggers.begin(); itr != m_ThreadDebuggers.end(); ++itr)
		{
			if ((*itr)->ToggleBreakPoint(filename, line))
				return;
		}
	}
	
	// If the breakpoint isn't handled yet search the breakpoints registered in this class
	std::list<CBreakPoint>* pBreakPoints = NULL;
	double breakPointsLockID = AquireBreakPointAccess(&pBreakPoints);

	// If set, delete
	bool deleted = false;
	for (std::list<CBreakPoint>::iterator itr = pBreakPoints->begin(); itr != pBreakPoints->end(); ++itr)
	{
		if ((*itr).m_Filename == filename && (*itr).m_UserLine == line)
		{
			itr = pBreakPoints->erase(itr);
			deleted = true;
			break;
		}
	}

	// If not set, set
	if (!deleted)
	{
		CBreakPoint bP;
		bP.m_Filename = filename;
		bP.m_UserLine = line;
		pBreakPoints->push_back(bP);
	}
	
	ReleaseBreakPointAccess(breakPointsLockID);
	return;
}

double CDebuggingServer::AquireBreakPointAccess(std::list<CBreakPoint>** breakPoints)
{
	int ret;
	ret = SDL_SemWait(m_BreakPointsSem);
	ENSURE(ret == 0);
	(*breakPoints) = &m_BreakPoints;
	m_BreakPointsLockID = timer_Time();
	return m_BreakPointsLockID;
}

void CDebuggingServer::ReleaseBreakPointAccess(double breakPointsLockID)
{
	ENSURE(m_BreakPointsLockID == breakPointsLockID); 
	SDL_SemPost(m_BreakPointsSem);
}
