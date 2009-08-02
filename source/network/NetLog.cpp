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

/**
 *-----------------------------------------------------------------------------
 *	FILE			: NetLog.cpp
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network subsystem logging classes implementation
 *-----------------------------------------------------------------------------
 */

// INCLUDES
#include "precompiled.h"
#include "NetLog.h"
#include "ps/CConsole.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

//-----------------------------------------------------------------------------
// Name: CNetLogEvent()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetLogEvent::CNetLogEvent( 
						   LogLevel level, 
						   const CStr& message, 
						   const CStr& loggerName )
{
	m_Level			= level;
	m_Message		= message;
	m_LoggerName	= loggerName;
}

//-----------------------------------------------------------------------------
// Name: ~CNetLogEvent()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetLogEvent::~CNetLogEvent( void )
{
}

//-----------------------------------------------------------------------------
// Name: CNetLogSink()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetLogSink::CNetLogSink( void )
{
	m_Active = false;
}

//-----------------------------------------------------------------------------
// Name: ~CNetLogSink()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetLogSink::~CNetLogSink( void )
{
}

//-----------------------------------------------------------------------------
// Name: SetName()
// Desc: Set the name for the sink
//-----------------------------------------------------------------------------
void CNetLogSink::SetName( const CStr& name )
{
	m_Name = name;
}

//-----------------------------------------------------------------------------
// Name: SetLevel()
// Desc: Set the level for the sink
//-----------------------------------------------------------------------------
void CNetLogSink::SetLevel( LogLevel level )
{
	m_Level = level;
}

//-----------------------------------------------------------------------------
// Name: SetHeader()
// Desc: Set new header text
//-----------------------------------------------------------------------------
void CNetLogSink::SetHeader( const CStr& header )
{
	m_Header = header;
}

//-----------------------------------------------------------------------------
// Name: SetFooter()
// Desc: Set new footer text
//-----------------------------------------------------------------------------
void CNetLogSink::SetFooter( const CStr& footer )
{
	m_Footer = footer;
}

//-----------------------------------------------------------------------------
// Name: Activate()
// Desc: Activates the sink
//-----------------------------------------------------------------------------
void CNetLogSink::Activate( void )
{
	CScopeLock lock( m_Mutex );

	if ( !m_Active )
	{
		OnActivate();

		m_Active = true;
	}
}

//-----------------------------------------------------------------------------
// Name: Close()
// Desc: Closes the sink
//-----------------------------------------------------------------------------
void CNetLogSink::Close( void )
{
	CScopeLock lock( m_Mutex );

	OnClose();
}

//-----------------------------------------------------------------------------
// Name: DoSink()
// Desc: Perform event logging
//-----------------------------------------------------------------------------
void CNetLogSink::DoSink( const CNetLogEvent& event )
{
	CScopeLock lock( m_Mutex );

	// Not activated? Nothing to log
	if ( !m_Active ) return;

	if ( TestEvent( event ) )
	{
		Sink( event );
	}
}

//-----------------------------------------------------------------------------
// Name: DoBulkSink()
// Desc: Perform logging for a list of events
//-----------------------------------------------------------------------------
void CNetLogSink::DoBulkSink( const CNetLogEvent* pEvents, size_t eventCount )
{
	size_t*	pIndices	= NULL;
	size_t	indexCount  = 0;
	size_t	i;

	CScopeLock lock( m_Mutex );

	// Validate parameters
	if ( !pEvents ) return;

	// Not activated? Nothing to log
	if ( m_Closed ) return;

	// Allocate new array which will store the events that will be logged
	pIndices = new size_t[ eventCount ];
	if ( !pIndices ) 
	{
		throw std::bad_alloc();
		return;
	}

	// Filter each event and store the index for
	// those passing the filter test
	for ( i = 0; i < eventCount; i++ )
	{
		if ( TestEvent( pEvents[ i ] ) )
		{
			pIndices[ indexCount++ ] = i;
		}
	}

	// Log each event
	for ( i = 0; i < indexCount; i++ )
	{
		Sink( pEvents[ pIndices[ i ] ] );
	}

	delete [] pIndices;
}

//-----------------------------------------------------------------------------
// Name: CNetLogSink()
// Desc: Test if the event can be logged
//-----------------------------------------------------------------------------
bool CNetLogSink::TestEvent( const CNetLogEvent& event )
{
	return ( event.GetLevel() >= m_Level );
}

//-----------------------------------------------------------------------------
// Name: WriteHeader()
// Desc: Writes a header
//-----------------------------------------------------------------------------
void CNetLogSink::WriteHeader( void )
{
	if ( !m_Header.empty() )
	{
		Write( m_Header );
	}
}

//-----------------------------------------------------------------------------
// Name: WriteFooter()
// Desc: Writes a footer
//-----------------------------------------------------------------------------
void CNetLogSink::WriteFooter( void )
{
	if ( !m_Footer.empty() )
	{
		Write( m_Footer );
	}
}

//-----------------------------------------------------------------------------
// Name: OnActivate()
// Desc: Called on activation
//-----------------------------------------------------------------------------
void CNetLogSink::OnActivate( void )
{
	// Does nothing by default
}

//-----------------------------------------------------------------------------
// Name: OnClose()
// Desc: Called on sink closure
//-----------------------------------------------------------------------------
void CNetLogSink::OnClose( void )
{
	// Does nothing by default
}

//-----------------------------------------------------------------------------
// Name: CNetLogFileSink()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetLogFileSink::CNetLogFileSink( void )
{
	// Get string time
	CStr time;
	CNetLogger::GetStringTime( time );
	
	// Make relative path
	fs::path path(fs::path(psLogDir())/"net_log");
	path /= time+".txt";
	m_FileName = path.external_file_string();
	m_Append = true;
}

//-----------------------------------------------------------------------------
// Name: CNetLogFileSink()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetLogFileSink::CNetLogFileSink( const CStr& filename )
{
	m_FileName	= filename;
	m_Append	= true;
}

//-----------------------------------------------------------------------------
// Name: CNetLogFileSink()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetLogFileSink::CNetLogFileSink( const CStr& filename, bool append )
{
	m_FileName	= filename;
	m_Append	= append;
}

//-----------------------------------------------------------------------------
// Name: ~CNetLogFileSink()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetLogFileSink::~CNetLogFileSink( void )
{
}

//-----------------------------------------------------------------------------
// Name: Activate()
// Desc: Open the file which will be used for logging
//-----------------------------------------------------------------------------
void CNetLogFileSink::OnActivate( void )
{
	OpenFile( m_FileName, m_Append );

	WriteHeader();
}

//-----------------------------------------------------------------------------
// Name: OnClose()
// Desc: Closes the file used for logging
//-----------------------------------------------------------------------------
void CNetLogFileSink::OnClose( void )
{
	WriteFooter();

	CloseFile();
}

//-----------------------------------------------------------------------------
// Name: Sink()
// Desc: Log the event to file
//-----------------------------------------------------------------------------
void CNetLogFileSink::Sink( const CNetLogEvent& event )
{
	Write( event.GetMessage() );
}

//-----------------------------------------------------------------------------
// Name: Write()
// Desc: Writes a message to log file
//-----------------------------------------------------------------------------
void CNetLogFileSink::Write( const CStr& message )
{
	// File not opened?
	if ( !m_File.is_open() )
	{
		OpenFile( m_FileName, m_Append );

		// If still not opened, ignore message
		if ( !m_File.is_open() ) return;
	}

	// Write message
	if ( !message.empty() )
	{
		m_File << message;
	}
}

//-----------------------------------------------------------------------------
// Name: Write()
// Desc: Writes a character to the file
//-----------------------------------------------------------------------------
void CNetLogFileSink::Write( char c )
{
	// File not opened?
	if ( !m_File.is_open() )
	{
		OpenFile( m_FileName, m_Append );

		// If still not opened, ignore character
		if ( !m_File.is_open() ) return;
	}

	// Write character
	m_File << c;
}

//-----------------------------------------------------------------------------
// Name: OpenFile()
// Desc: Open the file where the logging will output
//-----------------------------------------------------------------------------
void CNetLogFileSink::OpenFile( const CStr& fileName, bool append )
{
	// Close any open file
	if ( m_File.is_open() ) m_File.close();

	// Open the file and log start
	m_File.open( fileName.c_str(), append ? std::ios::app : std::ios::out );
	if ( !m_File.is_open() )
	{
		// throw std::ios_base::failure
		return;
	}

	m_FileName	= fileName;
	m_Append	= append;
}

//-----------------------------------------------------------------------------
// Name: CloseFile()
// Desc: Closes the opened file
//-----------------------------------------------------------------------------
void CNetLogFileSink::CloseFile( void )
{
	if ( m_File.is_open() ) 
	{
		m_File.flush();
		m_File.close();
	}
}

//-----------------------------------------------------------------------------
// Name: CNetLogConsoleSink()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetLogConsoleSink::CNetLogConsoleSink( void )
{
}

//-----------------------------------------------------------------------------
// Name: ~CNetLogConsoleSink()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetLogConsoleSink::~CNetLogConsoleSink( void )
{
}

//-----------------------------------------------------------------------------
// Name: Activate()
// Desc: Activates the game console
//-----------------------------------------------------------------------------
void CNetLogConsoleSink::OnActivate( void )
{
	WriteHeader();
}

//-----------------------------------------------------------------------------
// Name: OnClose()
// Desc: Toggles off game console
//-----------------------------------------------------------------------------
void CNetLogConsoleSink::OnClose( void )
{
	WriteFooter();
}

//-----------------------------------------------------------------------------
// Name: Sink()
// Desc: Log the event to file
//-----------------------------------------------------------------------------
void CNetLogConsoleSink::Sink( const CNetLogEvent& event )
{
	Write( event.GetMessage() );
}

//-----------------------------------------------------------------------------
// Name: Write()
// Desc: Writes a message to game console
//-----------------------------------------------------------------------------
void CNetLogConsoleSink::Write( const CStr& message )
{
	// Do we have a valid console?
	if ( !g_Console ) return;

	// Write message
	if ( !message.empty() )
	{	
		g_Console->InsertMessage( message.FromUTF8().c_str() );
	}
}

//-----------------------------------------------------------------------------
// Name: Write()
// Desc: Writes a character to the file
//-----------------------------------------------------------------------------
void CNetLogConsoleSink::Write( char c )
{
	// Do we have a valid console?
	if ( !g_Console ) return;

	// Write character
	Write( CStr( c ) );
}

//-----------------------------------------------------------------------------
// Name: CNetLogger()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetLogger::CNetLogger( const CStr& name )
{
	m_Name = name;
	m_Level = LOG_LEVEL_ALL;
}

//-----------------------------------------------------------------------------
// Name: ~CNetLogger()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetLogger::~CNetLogger( void )
{
}

//-----------------------------------------------------------------------------
// Name: SetLevel()
// Desc: Set logger level
//-----------------------------------------------------------------------------
void CNetLogger::SetLevel( LogLevel level )
{
	m_Level = level;
}

//-----------------------------------------------------------------------------
// Name: IsDebugEnabled()
// Desc: Check if enabled for DEBUG level
//-----------------------------------------------------------------------------
bool CNetLogger::IsDebugEnabled( void ) const
{
	return ( LOG_LEVEL_DEBUG >= m_Level );
}

//-----------------------------------------------------------------------------
// Name: IsErrorEnabled()
// Desc: Check if enabled for ERROR level
//-----------------------------------------------------------------------------
bool CNetLogger::IsErrorEnabled( void ) const
{
	return ( LOG_LEVEL_ERROR >= m_Level );
}

//-----------------------------------------------------------------------------
// Name: IsFatalEnabled()
// Desc: Check if enabled for FATAL level
//-----------------------------------------------------------------------------
bool CNetLogger::IsFatalEnabled( void ) const
{
	return ( LOG_LEVEL_FATAL >= m_Level );
}

//-----------------------------------------------------------------------------
// Name: IsInfoEnabled()
// Desc: Check if enabled for INFO level
//-----------------------------------------------------------------------------
bool CNetLogger::IsInfoEnabled( void ) const
{
	return ( LOG_LEVEL_INFO >= m_Level );
}

//-----------------------------------------------------------------------------
// Name: IsWarnEnabled()
// Desc: Check if enabled for WARN level
//-----------------------------------------------------------------------------
bool CNetLogger::IsWarnEnabled( void ) const
{
	return ( LOG_LEVEL_WARN >= m_Level );
}

//-----------------------------------------------------------------------------
// Name: Debug()
// Desc: Log a message with DEBUG level
//-----------------------------------------------------------------------------
void CNetLogger::Debug( const CStr& message )
{
	if ( IsDebugEnabled() )
	{
		// Get timestamp as a string
		CStr timer;
		GetStringTimeStamp( timer );

		CStr eventMessage	= "DEBUG";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= message;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_DEBUG, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Debug()
// Desc: Log a formatted message with DEBUG level
//-----------------------------------------------------------------------------
void CNetLogger::DebugFormat( const char* pFormat, ... )
{
	if ( IsDebugEnabled() )
	{
		char	buffer[ 512 ] = { 0 };
		CStr	timer;
		va_list	args;

		// Get timestamp as a string
		GetStringTimeStamp( timer );

		// Get arguments as a string
		va_start	( args, pFormat );
		vsnprintf	( buffer, 512, pFormat, args );
		va_end		( args );

		// Format message (e.g. ERROR - Hello World)
		CStr eventMessage	= "DEBUG";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= buffer;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_DEBUG, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Error()
// Desc: Log a message with ERROR level
//-----------------------------------------------------------------------------
void CNetLogger::Error( const CStr& message )
{
	if ( IsErrorEnabled() )
	{
		// Get timestamp as a string
		CStr timer;
		GetStringTimeStamp( timer );

		CStr eventMessage	= "ERROR";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= message;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_ERROR, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Error()
// Desc: Log a formatted message with ERROR level
//-----------------------------------------------------------------------------
void CNetLogger::ErrorFormat( const char* pFormat, ... )
{
	if ( IsErrorEnabled() )
	{
		char	buffer[ 512 ] = { 0 };
		CStr	timer;
		va_list	args;

		// Get timestamp as a string
		GetStringTimeStamp( timer );

		// Get arguments as a string
		va_start	( args, pFormat );
		vsnprintf	( buffer, 512, pFormat, args );
		va_end		( args );

		// Format message (e.g. ERROR - Hello World)
		CStr eventMessage	= "ERROR";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= buffer;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_ERROR, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Fatal()
// Desc: Log a message with FATAL level
//-----------------------------------------------------------------------------
void CNetLogger::Fatal( const CStr& message )
{
	if ( IsFatalEnabled() )
	{
		// Get timestamp as a string
		CStr timer;
		GetStringTimeStamp( timer );

		CStr eventMessage	= "FATAL";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= message;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_FATAL, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Fatal()
// Desc: Log a formatted message with FATAL error
//-----------------------------------------------------------------------------
void CNetLogger::FatalFormat( const char* pFormat, ... )
{
	if ( IsFatalEnabled() )
	{
		char	buffer[ 512 ] = { 0 };
		CStr	timer;
		va_list	args;

		// Get timestamp as a string
		GetStringTimeStamp( timer );

		// Get arguments as a string
		va_start	( args, pFormat );
		vsnprintf	( buffer, 512, pFormat, args );
		va_end		( args );

		// Format message (e.g. ERROR - Hello World)
		CStr eventMessage	= "FATAL";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= buffer;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_FATAL, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Info()
// Desc: Log a message with INFO level
//-----------------------------------------------------------------------------
void CNetLogger::Info( const CStr& message )
{
	if ( IsInfoEnabled() )
	{
		// Get timestamp as a string
		CStr timer;
		GetStringTimeStamp( timer );

		CStr eventMessage	= "INFO";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= message;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_INFO, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Info()
// Desc: Log a formatted message with INFO level
//-----------------------------------------------------------------------------
void CNetLogger::InfoFormat( const char* pFormat, ... )
{
	if ( IsInfoEnabled() )
	{
		char	buffer[ 512 ] = { 0 };
		CStr	timer;
		va_list	args;

		// Get timestamp as a string
		GetStringTimeStamp( timer );

		// Get arguments as a string
		va_start	( args, pFormat );
		vsnprintf	( buffer, 512, pFormat, args );
		va_end		( args );

		// Format message (e.g. ERROR - Hello World)
		CStr eventMessage	= "INFO";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= buffer;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_INFO, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Warn()
// Desc: Log a message with WARN level
//-----------------------------------------------------------------------------
void CNetLogger::Warn( const CStr& message )
{
	if ( IsWarnEnabled() )
	{
		// Get timestamp as a string
		CStr timer;
		GetStringTimeStamp( timer );

		CStr eventMessage	= "WARN";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= message;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_WARN, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: Warn()
// Desc: Log a formatted message with WARN level
//-----------------------------------------------------------------------------
void CNetLogger::WarnFormat( const char* pFormat, ... )
{
	if ( IsWarnEnabled() )
	{
		char	buffer[ 512 ] = { 0 };
		CStr	timer;
		va_list	args;

		// Get timestamp as a string
		GetStringTimeStamp( timer );

		// Get arguments as a string
		va_start	( args, pFormat );
		vsnprintf	( buffer, 512, pFormat, args );
		va_end		( args );

		// Format message (e.g. ERROR - Hello World)
		CStr eventMessage	= "WARN";
		eventMessage		+= " - ";
		eventMessage		+= timer;
		eventMessage		+= " ";
		eventMessage		+= buffer;
		eventMessage		+= "\n";

		CNetLogEvent newEvent( LOG_LEVEL_WARN, eventMessage, m_Name );

		CallSinks( newEvent );
	}
}

//-----------------------------------------------------------------------------
// Name: AddSink()
// Desc: Attaches a new sink
//-----------------------------------------------------------------------------
void CNetLogger::AddSink( CNetLogSink* pSink )
{
	size_t		i;
	CScopeLock	lock( m_Mutex );

	assert( pSink );

	// Validate parameter
	if ( !pSink ) return;

	// Check if already exists
	for ( i = 0; i < GetSinkCount(); i++ )
	{
		CNetLogSink* pCurrSink = GetSink( i );
		if ( pCurrSink == pSink ) break;
	}
	
	// Already exists?
	if ( i >= GetSinkCount() )
	{
		// Activate new sink
		pSink->Activate();

		// Add new sink to list
		m_Sinks.push_back( pSink );
	}
}

//-----------------------------------------------------------------------------
// Name: RemoveSink()
// Desc: Removes the sink from the list of attached sinks
//-----------------------------------------------------------------------------
CNetLogSink* CNetLogger::RemoveSink( CNetLogSink* pSink )
{
	CScopeLock lock( m_Mutex );

	// Validate parameter
	if ( !pSink ) return NULL;

	// Lookup the sink object
	SinkList::iterator it = m_Sinks.begin();
	for ( ; it != m_Sinks.end(); it++ )
	{
		CNetLogSink* pCurrSink = *it;
		if ( pCurrSink == pSink )
		{
			m_Sinks.erase( it );

			return pCurrSink;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Name: RemoveSink()
// Desc: Removes the named sink
//-----------------------------------------------------------------------------
CNetLogSink* CNetLogger::RemoveSink( const CStr& name )
{
	CScopeLock lock( m_Mutex );

	// Lookup sink by name
	SinkList::iterator it = m_Sinks.begin();
	for ( ; it != m_Sinks.end(); it++ )
	{
		CNetLogSink* pCurrSink = *it;
		if ( !pCurrSink ) continue;

		if ( pCurrSink->GetName() == name )
		{
			m_Sinks.erase( it );

			return pCurrSink;
		}
	}

	// Sink not found
	return NULL;
}

//-----------------------------------------------------------------------------
// Name: RemoveAllSinks()
// Desc: Remove all attached sinks
//-----------------------------------------------------------------------------
void CNetLogger::RemoveAllSinks( void )
{
	CScopeLock lock( m_Mutex );

	m_Sinks.clear();
}

//-----------------------------------------------------------------------------
// Name: GetSinkCount()
// Desc: Retrive the number of attached sinks
//-----------------------------------------------------------------------------
size_t CNetLogger::GetSinkCount( void )
{
	return m_Sinks.size();
}

//-----------------------------------------------------------------------------
// Name: GetSink()
// Desc: Retrieves the sink by index
//-----------------------------------------------------------------------------
CNetLogSink* CNetLogger::GetSink( size_t index )
{
	// Validate parameter
	if ( index > m_Sinks.size() ) return NULL;

	return m_Sinks[ index ];
}

//-----------------------------------------------------------------------------
// Name: GetSink()
// Desc: Retrieve the sink by name
//-----------------------------------------------------------------------------
CNetLogSink* CNetLogger::GetSink( const CStr& name )
{
	for ( size_t i = 0; i < GetSinkCount(); i++ )
	{
		CNetLogSink* pCurrSink = GetSink( i );
		if ( !pCurrSink ) continue;

		if ( pCurrSink->GetName() == name )
			return pCurrSink;
	}

	// Sink not found
	return NULL;
}

//-----------------------------------------------------------------------------
// Name: CallSinks()
// Desc:
//-----------------------------------------------------------------------------
void CNetLogger::CallSinks( const CNetLogEvent& event )
{
	for ( size_t i = 0; i < GetSinkCount(); i++ )
	{
		CNetLogSink* pCurrSink = GetSink( i );
		if ( !pCurrSink ) continue;

		pCurrSink->DoSink( event );
	}
}

//-----------------------------------------------------------------------------
// Name: GetStringLocalTime()
// Desc: Returns the local date time into the passed in string
//-----------------------------------------------------------------------------
void CNetLogger::GetStringDateTime( CStr &str )
{
	char		buffer[ 128 ] = { 0 };
	time_t		tm;
	struct tm       *now;

#if OS_WIN
	// Set timezone
	_tzset();

	// Get time and convert to tm structure
	time( &tm );
	struct tm nowBuf;
	localtime_s( &nowBuf, &tm );
	now = &nowBuf;
#else
	time ( &tm );
	now = localtime( &tm );
#endif
	
	// Build custom time string
	strftime( buffer, 128, "%Y-%m-%d %H:%M:%S", now );

	str = buffer;
}

//-----------------------------------------------------------------------------
// Name: GetStringTime()
// Desc: Returns the local time into the passed string
//-----------------------------------------------------------------------------
void CNetLogger::GetStringTime( CStr& str )
{
	char		buffer[ 128 ] = { 0 };
	time_t		tm;
	struct tm       *now;

#if OS_WIN
	// Set timezone
	_tzset();

	// Get time and convert to tm structure
	time( &tm );
	struct tm nowBuf;
	localtime_s( &nowBuf, &tm );
	now = &nowBuf;
#else
	time ( &tm );
	now = localtime( &tm );
#endif

	// Build custom time string
	strftime( buffer, 128, "%H%M%S", now );

	str = buffer;
}

//-----------------------------------------------------------------------------
// Name: GetStringTimeStamp()
// Desc: Returns the formatted current time into the passed string
//-----------------------------------------------------------------------------
void CNetLogger::GetStringTimeStamp( CStr& str )
{
	char buffer[ 128 ] = { 0 };
	double timestamp = timer_Time();
	sprintf( buffer, "[%3u.%03u]", ( unsigned )timestamp, ( ( unsigned )( timestamp * 1000 ) % 1000 ) );
	str = buffer;
}

// List of loggers under log manager
LoggerList CNetLogManager::m_Loggers;

//-----------------------------------------------------------------------------
// Name: Shutdown()
// Desc: Shuts down the log manager
//-----------------------------------------------------------------------------
void CNetLogManager::Shutdown( void )
{
	// Remove all loggers
	LoggerList::iterator it = m_Loggers.begin();
	for ( ; it != m_Loggers.end(); it++ )
	{
		CNetLogger *pCurrLogger = *it;
		if ( !pCurrLogger ) continue;

		pCurrLogger->RemoveAllSinks();

		delete pCurrLogger;
	}

	m_Loggers.clear();
}

//-----------------------------------------------------------------------------
// Name: GetLogger()
// Desc: Retrieve or create a named logger
//-----------------------------------------------------------------------------
CNetLogger* CNetLogManager::GetLogger( const CStr& name )
{
	LoggerList::const_iterator it = m_Loggers.begin();
	for ( ; it != m_Loggers.end(); it++ )
	{
		CNetLogger* pCurrLogger = *it;
		if ( !pCurrLogger ) continue;

		if ( pCurrLogger->GetName() == name )
			return pCurrLogger;
	}

	// Logger not found, create it
	CNetLogger* pNewLogger = new CNetLogger( name );
	if ( !pNewLogger ) return NULL;

	// Add new logger to list
	m_Loggers.push_back( pNewLogger );

	return pNewLogger;
}

//-----------------------------------------------------------------------------
// Name: GetAllLoggers()
// Desc: Return the list of loggers
//-----------------------------------------------------------------------------
const LoggerList& CNetLogManager::GetAllLoggers( void )
{
	return m_Loggers;
}


