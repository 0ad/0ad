#include "precompiled.h"

#include "CLogger.h"
#include "ConfigDB.h"
#include "lib.h"

#include "CConsole.h"
extern CConsole* g_Console;

using namespace std;

const char* html_header0 =
	"<HTML>\n<HEAD>\n<LINK REL=StyleSheet HREF="
	"\"style.css\" TYPE=\"text/css\">\n"
	"</HEAD>\n<BODY>\n<P align=\"center\"><IMG src="
	"\"0adlogo.jpg\"/></P>\n"
	"<P><H1>";

const char* html_header1 = "</H1></P>\n";

const char* html_footer = "</BODY>\n</HTML>\n";

#define MEMORY_BUFFER_SIZE	240

CLogger * CLogger::GetInstance()
{
	static CLogger m_Instance;

	return &m_Instance;
}

CLogger::CLogger()
{

	m_NumberOfMessages = 0;
	m_NumberOfErrors = 0;
	m_NumberOfWarnings = 0;
	
	m_MemoryLogBuffer = (char *)calloc(MEMORY_BUFFER_SIZE, 1);
	m_CurrentPosition = m_MemoryLogBuffer;

	// current directory is $install_dir/data, we want $install_dir/logs.
	// TODO: make sure we are called after file_rel_chdir,
	// or else cur dir may be anywhere
	m_MainLog.open			("../logs/mainlog.html",		ofstream::out | ofstream::trunc);
	m_InterestingLog.open	("../logs/interestinglog.html",	ofstream::out | ofstream::trunc);
	m_MemoryLog.open		("../logs/memorylog.html",		ofstream::out | ofstream::trunc);

	//Write Headers for the HTML documents
	m_MainLog << html_header0 << "Main log" << html_header1;
	
	//Write Headers for the HTML documents
	m_InterestingLog << html_header0 << "Main log (interesting items only, as specified in system.cfg)" << html_header1;

}

CLogger::~CLogger ()
{
	char buffer[60];
	sprintf(buffer," with %d message(s), %d error(s) and %d warning(s).", \
						m_NumberOfMessages,m_NumberOfErrors,m_NumberOfWarnings);

	time_t t = time(NULL);
	struct tm* now = localtime(&t);
	const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	CStr currentDate = CStr(months[now->tm_mon]) + " " + CStr(now->tm_mday) + " " + CStr(1900+now->tm_year);
	char currentTime[10];
	sprintf(currentTime, "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);

	//Write closing text

	m_MainLog << "<P>Engine exited successfully on " << currentDate;
	m_MainLog << " at " << currentTime << buffer << "</P>\n";
	m_MainLog << html_footer;
	m_MainLog.close ();
	
	m_InterestingLog << "<P>Engine exited successfully on " << currentDate;
	m_InterestingLog << " at " << currentTime << buffer << "</P>\n";
	m_InterestingLog << html_footer;
	m_InterestingLog.close ();


	//Add end marker to logs in memory
	m_CurrentPosition = '\0';

	m_MemoryLog << html_header0 << "Memory log" << html_header1;
	m_MemoryLog << "<P>Memory Log started with capacity of " << \
						MEMORY_BUFFER_SIZE << " characters.</P>\n";
	m_MemoryLog << m_MemoryLogBuffer;
	m_MemoryLog << html_footer;

	m_MemoryLog.close ();

	free(m_MemoryLogBuffer);
}

void CLogger::WriteMessage(const char *message, int interestedness)
{
	m_NumberOfMessages++;

	if (interestedness >= 2)
	{
		if (g_Console) g_Console->InsertMessage(L"LOG: %hs", message);
		m_InterestingLog << "<P>" << message << "</P>\n";
		m_InterestingLog.flush();
	}
	m_MainLog << "<P>" << message << "</P>\n";
	m_MainLog.flush();
	
}

void CLogger::WriteError(const char *message, int interestedness)
{
	m_NumberOfErrors++;
	debug_out("ERROR: %s\n", message);
	if (interestedness >= 1)
	{
		if (g_Console) g_Console->InsertMessage(L"ERROR: %hs", message);
		m_InterestingLog << "<P class=\"error\">ERROR: "<< message << "</P>\n";
		m_InterestingLog.flush();
	}
	m_MainLog << "<P class=\"error\">ERROR: "<< message << "</P>\n";
	m_MainLog.flush();
}

void CLogger::WriteWarning(const char *message, int interestedness)
{
	m_NumberOfWarnings++;
	if (interestedness >= 1)
	{
		if (g_Console) g_Console->InsertMessage(L"WARNING: %hs", message);
		m_InterestingLog << "<P class=\"warning\">WARNING: "<< message << "</P>\n";
		m_InterestingLog.flush();
	}
	m_MainLog << "<P class=\"warning\">WARNING: "<< message << "</P>\n";
	m_MainLog.flush();
}

// Sends the message to the appropriate piece of code
void CLogger::LogUsingMethod(ELogMethod method, const char* category, const char* message)
{
	if(method == NORMAL)
		WriteMessage(message, Interestedness(category));
	else if(method == ERROR)
		WriteError(message, Interestedness(category));
	else if(method == WARNING)
		WriteWarning(message, Interestedness(category));
	else
		WriteMessage(message, Interestedness(category));
}

void CLogger::Log(ELogMethod method, const char* category, const char *fmt, ...)
{
	va_list argp;
	char buffer[512];

	memset(buffer,0,sizeof(buffer));
	
	va_start(argp, fmt);
	vsnprintf2(buffer, sizeof(buffer), fmt, argp);
	va_end(argp);

	LogUsingMethod(method, category, buffer);
}


void CLogger::LogOnce(ELogMethod method, const char* category, const char *fmt, ...)
{
	va_list argp;
	char buffer[512];

	memset(buffer,0,sizeof(buffer));

	va_start(argp, fmt);
	vsnprintf2(buffer, sizeof(buffer), fmt, argp);
	va_end(argp);

	std::string message (buffer);

	// If this message has already been logged, ignore it
	if (m_LoggedOnce.find(message) != m_LoggedOnce.end())
		return;

	// If not, mark it as having been logged and then log it
	m_LoggedOnce.insert(message);
	LogUsingMethod(method, category, buffer);
}


void CLogger::QuickLog(const char *fmt, ...)
{
	va_list argp;
	char buffer[512];
	int count = 0;

	//Start a new paragraph in HTML
	strcpy(buffer,"<P>");

	va_start(argp, fmt);
	vsnprintf(strchr(buffer, 0), sizeof(buffer), fmt, argp);
	va_end(argp);

	//add some html formatting
	strcat(buffer,"</P>");
	
	if((m_CurrentPosition - m_MemoryLogBuffer + strlen(buffer) + 1) >= MEMORY_BUFFER_SIZE)
	{
		//not enough room in the buffer so don't log.
		return;
	}

	while(buffer[count] != '\0')
	{
		*m_CurrentPosition = buffer[count];
		*m_CurrentPosition++;
		count++;
	}
	
	*m_CurrentPosition = '\n';
	*m_CurrentPosition++;
	
	free(buffer);
}

int CLogger::Interestedness(const char* category)
{
	// This could be cached, but reading from the config DB every time allows
	// easy run-time alteration of interest levels (and shouldn't be particularly
	// slow)

	// Category unspecified: use a high interest level to encourage
	// people to categorise their errors
	if (category == NULL)
		return 2;

	// If the config DB hasn't been loaded, assume the default
	if (! CConfigDB::IsInitialised())
		return 1;

	CConfigValue* v = g_ConfigDB.GetValue(CFG_SYSTEM, CStr("loginterest.")+category);
	// If the value is unspecified, also use the default
	if (!v)
		return 1;

	int level;
	// Something failed, so the default value might be a good alternative
	if (! v->GetInt(level))
		return 1;

	return level;
}
