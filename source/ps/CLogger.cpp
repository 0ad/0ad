#include "precompiled.h"

#include "CLogger.h"
#include "lib.h"

#define CONSOLE_DEBUG

#ifdef CONSOLE_DEBUG
 #include "CConsole.h"
 extern CConsole* g_Console;
#endif

using namespace std;

#define MAIN_HEADER		"<HTML>\n<HEAD>\n<LINK REL=StyleSheet HREF=" \
						"\"style.css\" TYPE=\"text/css\">\n" \
						"</HEAD>\n<BODY>\n<P align=\"center\"><IMG src=" \
						"\"0adlogo.jpg\"/></P>\n" \
						"<P><H1>Main Log</H1></P>\n"

#define MEMORY_HEADER	"<HTML>\n<HEAD>\n<LINK REL=StyleSheet HREF=" \
						"\"style.css\" TYPE=\"text/css\">\n" \
						"</HEAD>\n<BODY>\n<P align=\"center\"><IMG src=" \
						"\"0adlogo.jpg\"/></P>\n" \
						"<P><H1>Memory Log</H1></P>\n"


#define FOOTER			"</BODY>\n</HTML>\n"

#define MEMORY_BUFFER_SIZE	35

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
	
	m_MemoryLog = (char *) malloc(MEMORY_BUFFER_SIZE);
	m_CurrentPosition = m_MemoryLog;

	memset(m_MemoryLog,0,MEMORY_BUFFER_SIZE);

	// current directory is $install_dir/data, we want $install_dir/logs.
	// TODO: make sure we are called after file_rel_chdir,
	// or else cur dir may be anywhere
	m_MainLog.open ("../logs/mainlog.html",ofstream::out | ofstream::trunc);
	m_DetailedLog.open ("../logs/detailedlog.html",ofstream::out | ofstream::trunc );

	//Write Headers for the HTML documents
	m_MainLog << MAIN_HEADER;
	
}

CLogger::~CLogger ()
{
	char buffer[60];
	sprintf(buffer," with %d message(s), %d error(s) and %d warning(s).", \
						m_NumberOfMessages,m_NumberOfErrors,m_NumberOfWarnings);

	//Write closing text
	m_MainLog << "<P>Engine exited successfully on " << __DATE__;
	m_MainLog << " at " << __TIME__ << buffer << "</P>\n";
	m_MainLog << FOOTER;
	
	//Add end marker to logs in memory
	m_CurrentPosition = '\0';

	m_DetailedLog << MEMORY_HEADER;
	m_DetailedLog << "<P>Memory Log started with capacity of " << \
						MEMORY_BUFFER_SIZE << " characters.</P>\n";
	m_DetailedLog << m_MemoryLog;
	m_DetailedLog << FOOTER;

	m_DetailedLog.close ();

	m_MainLog.close ();

	free(m_MemoryLog);
}

void CLogger::WriteMessage(const char *message)
{
#ifdef CONSOLE_DEBUG
	g_Console->InsertMessage(L"LOG: %hs", message);
#endif
	m_NumberOfMessages++;
	m_MainLog << "<P>" << message << "</P>";
									
	m_MainLog.flush();
	
}

void CLogger::WriteError(const char *message)
{
#ifdef CONSOLE_DEBUG
	g_Console->InsertMessage(L"ERROR: %hs", message);
#endif
	debug_out("ERROR: %s\n", message);
	m_NumberOfErrors++;
	m_MainLog << "<P class=\"error\">ERROR: "<< message << "</P>\n";
	m_MainLog.flush();
}

void CLogger::WriteWarning(const char *message)
{
#ifdef CONSOLE_DEBUG
	g_Console->InsertMessage(L"WARNING: %hs", message);
#endif
	m_NumberOfWarnings++;
	m_MainLog << "<P class=\"warning\">WARNING: "<< message << "</P>\n";

	m_MainLog.flush();
}

void CLogger::Log(ELogMethod method, const char *fmt, ...)
{
	va_list argp;
	char buffer[512];

	memset(buffer,0,sizeof(buffer));
	
	va_start(argp, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, argp);
	va_end(argp);

	if(method == NORMAL)
		WriteMessage(buffer);
	else if(method == ERROR)
		WriteError(buffer);
	else if(method == WARNING)
		WriteWarning(buffer);
	else
		WriteMessage(buffer);
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
	
	if((m_CurrentPosition - m_MemoryLog + strlen(buffer) + 1) >= MEMORY_BUFFER_SIZE)
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

