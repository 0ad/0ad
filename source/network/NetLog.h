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
 *	FILE			: NetLog.h
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network subsystem logging classes declarations
 *-----------------------------------------------------------------------------
 */

#ifndef NETLOG_H
#define NETLOG_H

// INCLUDES
#include "ps/Pyrogenesis.h"
#include "ps/ThreadUtil.h"
#include "ps/CStr.h"
#include "lib/timer.h"

#include <list>
#include <vector>
#include <fstream>

// DECLARATIONS
typedef enum
{
	LOG_LEVEL_ALL		= 0x0000,	// Lowest level possible
	LOG_LEVEL_DEBUG		= 0x0001,	// Informational events for app debugging
	LOG_LEVEL_INFO		= 0x0002,	// Useful for highlighting app progress
	LOG_LEVEL_WARN		= 0x0004,	// Potentially dangerous situations
	LOG_LEVEL_ERROR		= 0x0008,	// Error events but the app can continue
	LOG_LEVEL_FATAL		= 0x0010,	// Very severe errors, the app aborts
	LOG_LEVEL_OFF		= 0xFFFF,	// The highest level possible
} LogLevel;

class CNetLogger;
class CNetLogSink;

typedef std::list< CNetLogger* >		LoggerList;
typedef std::vector< CNetLogSink* >		SinkList;

/*
	CLASS			: CNetLogEvent
	DESCRIPTION		: CNetLogEvent represents an object passed between
					  different network logging components when a decision
					  for logging is made
	NOTES			: 
*/

class CNetLogEvent
{
public:

	CNetLogEvent( 
					LogLevel level, 
					const CStr& message, 
					const CStr& loggerName );
	~CNetLogEvent( void );

	/**
	 * Returns the level of the event
	 *
	 * @return					Event level;
	 */
	inline LogLevel GetLevel( void ) const		{ return m_Level; }

	/**
	 * Returns the time of the event
	 *
	 * @return					Event time
	 */
	inline TimerUnit GetTimeStamp( void ) const	{ return m_TimeStamp; }

	/**
	 * Returns the name of the logger which logged the event
	 *
	 * @return					Logger name
	 */
	inline const CStr& GetLoggerName( void ) const { return m_LoggerName; }

	/**
	 * Returns the message used when the event was initialized
	 *
	 * @return					Event message
	 */
	inline const CStr& GetMessage( void ) const { return m_Message; }

protected:

private:

	// Not implemented
	CNetLogEvent( const CNetLogEvent& );
	CNetLogEvent& operator=( CNetLogEvent& );

	LogLevel		m_Level;			// Current level
	CStr			m_LoggerName;		// Logger which processed the event
	CStr			m_Message;			// Application message for the event
	TimerUnit		m_TimeStamp;		// Event logging time
};

/*
	CLASS			: CNetLogSink
	DESCRIPTION		: CNetLogSink is the basic interface for events logging
	NOTES			:
*/

class CNetLogSink
{
public:

	CNetLogSink( );
	virtual ~CNetLogSink( );

	/**
	 * Set the name of the sink
	 *
	 * @param name				New sink name
	 */
	void SetName( const CStr& name );

	/**
	 * Retrieves the name of the sink
	 *
	 * @return					Sink name
	 */
	inline const CStr& GetName( void ) const { return m_Name; }

	/**
	 * Set the level of the sink
	 *
	 * @param level				New sink level
	 */
	void SetLevel( LogLevel level );

	/**
	 * Retrieves the current level of the sink
	 *
	 * @return					Sink current level
	 */
	inline LogLevel GetLevel( void ) const	{ return m_Level; }

	/**
	 * Retrieves the header text
	 *
	 * @return					The header text
	 */
	inline const CStr& GetHeader( void ) const { return m_Header; }

	/**
	 * Set header text which will be logged before an event logging
	 *
	 * @param header			New header text
	 */
	void SetHeader( const CStr& header );

	/**
	 * Retrieves the footer text
	 *
	 * @return					The footer text
	 */
	inline const CStr& GetFooter( void ) const { return m_Footer; }

	/**
	 * Set footer text which will be logged after an event logging
	 *
	 * @param footer			New footer text
	 */
	void SetFooter( const CStr& footer );

	/**
	 * Activates the sink
	 */
	void Activate( void );

	/**
	 * Closes the sink and release any resources
	 */
	void Close( void );

	/**
	 * Check if the level of the event is greater than or equal to sink level
	 * and if it succeeds it performs the actual logging of the event.
	 *
	 * @param event				Event to log
	 */
	void DoSink( const CNetLogEvent& event );

	/**
	 * For each event from the passed array, check if its level is greater than
	 * or equal to sink level and performs the logging for it.
	 *
	 * @param pEvents			List of events to log
	 * @param eventCount		The number of events in pEvents list
	 */
	void DoBulkSink( const CNetLogEvent* pEvents, size_t eventCount );

	/**
	 * Check if the sink can log the specified event
	 *
	 * @param event				Event to check
	 * @return					true if the event can be logged, 
	 *							false otherwise
	 */
	virtual bool TestEvent( const CNetLogEvent& event );

protected:

	/**
	 * Activates the sink object
	 */
	virtual void OnActivate( void );

	/**
	 * Writes a header into the sink
	 */
	virtual void WriteHeader( void );

	/**
	 * Writes a footer into the sink
	 */
	virtual void WriteFooter( void );

	/**
	 * Writes a string message to the logging output
	 *
	 * @param message			The message to log
	 */
	virtual void Write( const CStr& message ) = 0;

	/**
	 * Writes a single character to the logging output
	 *
	 * @param c					The character to log
	 */
	virtual void Write( char c ) = 0;

	/**
	 * This is called by Close method. It can be overriden by specialized sinks
	 * if any resources needs to be released on close.
	 */
	virtual void OnClose( void );

	/**
	 * This method is called by DoSink and DoBulkSink and it must be 
	 * implemented by specialized sinks to perform actual logging
	 *
	 * @param event				Event to log
	 */
	virtual void Sink( const CNetLogEvent& event ) = 0;

	LogLevel	m_Level;				// Current level
	CMutex		m_Mutex;				// Multithreading synchronization object
	CStr		m_Header;				// Header text
	CStr		m_Footer;				// Footer text
	CStr		m_Name;					// Sink name
	bool		m_Closed;				// Indicates whether the sink is closed
	bool		m_Active;				// Indicates whether the sink is active

private:

	// Not implemented
	CNetLogSink( const CNetLogSink& );
	CNetLogSink& operator=( const CNetLogSink& );
};

/*
	CLASS			: CNetLogFileSink
	DESCRIPTION		: Log network events to a file
	NOTES			: 
*/

class CNetLogFileSink : public CNetLogSink
{
public:

	CNetLogFileSink( void );
	CNetLogFileSink( const fs::wpath& filename );
	CNetLogFileSink( const fs::wpath& filename, bool append );
	~CNetLogFileSink( void );

protected:

	/**
	 * Activates the sink object and opens the file specified in constructor
	 */
	virtual void OnActivate( void );

	/**
	 * Closes the log file
	 */
	virtual void OnClose( void );

	/**
	 * Writes the event to the log file if opened
	 *
	 * @param event				Event to log
	 */
	virtual void Sink( const CNetLogEvent& event );

	/**
	 * Writes the message passed as parameter to file
	 *
	 * @param message			The message to log
	 */
	virtual void Write( const CStr& message );

	/**
	 * Writes the character passed as parameter to file
	 *
	 * @param c					The character to log
	 */
	virtual void Write( char c );

private:

	// Not implemented
	CNetLogFileSink( const CNetLogFileSink& );
	CNetLogFileSink& operator=( const CNetLogFileSink& );

	/**
	 * Open the file where logging goes. The header text will be written each
	 * time the file is opened. If append parameter is true, then the file may
	 * contain the header many times.
	 *
	 * @param filename			The path to the log file
	 * @param append			Indicates whether logging should append to 
	 *							the file or truncate the file
	 */
	void OpenFile( const fs::wpath& fileName, bool append );

	/**
	 * Close the previously opened file. The footer text will be written each time
	 * the file is closed. If the file was opened for appending, the footer might
	 * appera many times.
	 */
	void CloseFile( void );

	std::ofstream	m_File;				// The log file handle
	fs::wpath       m_FileName;			// The name of the log file
	bool			m_Append;			// Logging should append to file
};

/*
	CLASS			: CNetLogConsoleSink
	DESCRIPTION		: Log network events to the game console
	NOTES			: 
*/

class CNetLogConsoleSink : public CNetLogSink
{
public:

	CNetLogConsoleSink( void );
	~CNetLogConsoleSink( void );

protected:

	/**
	 * Activates the sink object and the game console
	 */
	virtual void OnActivate( void );

	/**
	 * Toggle off game console
	 */
	virtual void OnClose( void );

	/**
	 * Writes the event to the game console if active
	 *
	 * @param event				Event to log
	 */
	virtual void Sink( const CNetLogEvent& event );

	/**
	 * Writes the message passed as parameter to game console
	 *
	 * @param message			The message to log
	 */
	virtual void Write( const CStr& message );

	/**
	 * Writes the character passed as parameter to game console
	 *
	 * @param c					The character to log
	 */
	virtual void Write( char c );

private:

	// Not implemented
	CNetLogConsoleSink( const CNetLogConsoleSink& );
	CNetLogConsoleSink& operator=( const CNetLogConsoleSink& );
};

/*
	CLASS			: CNetLogger
	DESCRIPTION		: CNetLogger serves for logging messages for network subsytem.
					  It contains methods for logging at different levels.
	NOTES			: CNetLogManager is used to obtain an instance of a logger.
*/

class CNetLogger
{
public:

	CNetLogger( const CStr& name );
	virtual ~CNetLogger( void );

	bool	IsDebugEnabled	( void ) const;
	bool	IsInfoEnabled	( void ) const;
	bool	IsWarnEnabled	( void ) const;
	bool	IsErrorEnabled	( void ) const;
	bool	IsFatalEnabled	( void ) const;

	void	Debug			( const CStr& message );
	void	Warn			( const CStr& message );
	void	Info			( const CStr& message );
	void	Error			( const CStr& message );
	void	Fatal			( const CStr& message );

	void	DebugFormat		( const char* pFormat, ... ) PRINTF_ARGS(2);
	void	WarnFormat		( const char* pFormat, ... ) PRINTF_ARGS(2);
	void	InfoFormat		( const char* pFormat, ... ) PRINTF_ARGS(2);
	void	ErrorFormat		( const char* pFormat, ... ) PRINTF_ARGS(2);
	void	FatalFormat		( const char* pFormat, ... ) PRINTF_ARGS(2);

	/**
	 * Retrieves the name of the logger
	 *
	 * @return					Logger name
	 */
	const CStr& GetName( void ) const { return m_Name; }

	/**
	 * Retrieves the level of the logger
	 *
	 * @return					Logger level
	 */
	LogLevel GetLevel( void ) const	{ return m_Level; }

	/**
	 * Set the level for the logger
	 *
	 * @param level				New logger level
	 */
	void SetLevel( LogLevel level );

	/**
	 * Attaches a new sink to the list of sinks. The sink will be activated.
	 *
	 * @param pSink				The sink to add
	 */
	void AddSink( CNetLogSink* pSink );

    /**
	 * Removes the specified sink from the list of attached sinks. The sink
	 * will not be closed.
	 *
	 * @param pSink				The sink to remove
	 * @return					The removed sink or NULL if not found
	 */
	CNetLogSink* RemoveSink( CNetLogSink* pSink );

	/**
	 * Remove the named sink passed as parameter. The sink will not be closed.
	 *
	 * @param name				The name of sink to remove
	 * @return					The removed sink or NULL if not found
	 */
    CNetLogSink* RemoveSink( const CStr& name );

	/**
	 * Removes all attached sinks
	 *
	 */
	void RemoveAllSinks( void );

	/**
	 * Retrieve the number of attached sinks
	 *
	 * @return					The number of sink objects
	 */
	size_t GetSinkCount( void );

	/**
	 * Retrieves the sink by its index
	 *
	 * @param index				The index of the sink 
	 * @return					NULL if index is out of boundaries or
	 *							the sink at the specified index
	 */
	CNetLogSink* GetSink( size_t index );

	/**
	 * Retrieves a sink by its name
	 *
	 * @param name				The name of the sink
	 * @return					NULL if the sink does not exists or
	 *							the sink with the specified name
	 */
	CNetLogSink* GetSink( const CStr& name );

	/**
	 * Helper function used to retrieve local date time in a string
	 */
	static void GetStringDateTime( CStr& str );

	/**
	 * Helper function used to retrieve local time in a string
	 */
	static void GetStringTime( CStr& str );

	/**
	 * Helper function used to retrieve the current timestamp in a string
	 */
	static void GetStringTimeStamp( CStr& str );

protected:

private:

	// Not implemented
	CNetLogger( const CNetLogger& );
	CNetLogger& operator=( const CNetLogger& );

	/**
	 * Dispatch the event passed as parameter to all sinks
	 *
	 * @param event				The event to log
	 */
	void CallSinks( const CNetLogEvent& event );

	CMutex		m_Mutex;				// Multithread synchronization object
	SinkList	m_Sinks;				// Holds the list of sink objects
	LogLevel	m_Level;				// Logger level
	CStr		m_Name;					// Logger name
};

/*
	CLASS			: CNetLogManager
	DESCRIPTION		: CNetLogManager serves clients requesting log instances
	NOTES			: The GetLogger method can be used to retrieve a log
*/

class CNetLogManager
{
public:

	/**
	 * Shutdown the log manager, closes all sinks in the loggers.
	 *
	 */
	static void Shutdown( void );

	/**
	 * Retrieves a named logger. If the logger does not exist, it is created.
	 *
	 * @param name			Logger name
	 * @return				A logger object
	 */
	static CNetLogger* GetLogger( const CStr& name );

	/**
	 * Return the list of all defined loggers.
	 *
	 * @return				The list of all loggers
	 */
	static const LoggerList& GetAllLoggers( void );

private:

	// Not implemented
	CNetLogManager( void );
	~CNetLogManager( void );
	CNetLogManager( const CNetLogManager& );
	CNetLogManager& operator=( const CNetLogManager& );

	static LoggerList m_Loggers;		// Holds the list of loggers
};

// TODO: Replace with better access to log manager
#define START_LOGGER( sinkName, sinkType )\
	CNetLogger *pLogger = CNetLogManager::GetLogger( "net.log" );\
	if ( pLogger )\
	{\
		CNetLogSink* pSink = pLogger->GetSink( sinkName );\
		if ( !pSink )\
		{\
			pSink = new sinkType();\
			if ( pSink )\
			{\
				CStr startTime;\
				CNetLogger::GetStringDateTime( startTime );\
				CStr header = "***************************************************\n";\
				header += "LOG STARTED: ";\
				header += startTime;\
				header += "\n";\
				header += "Timestamps are in seconds since engine startup\n";\
				pSink->SetHeader( header );\
				pSink->SetName( sinkName );\
				if ( strcmp(sinkName, "sink.console") == 0 )\
					pSink->SetLevel( LOG_LEVEL_ERROR );\
				else\
					pSink->SetLevel( LOG_LEVEL_INFO );\
				pLogger->AddSink( pSink );\
			}\
		}

#define END_LOGGER\
	}

#define NET_LOG( parameter )\
	{\
		START_LOGGER( "sink.file", CNetLogFileSink )\
		pLogger->Info( parameter );\
		END_LOGGER\
	}\
	{\
		START_LOGGER( "sink.console", CNetLogConsoleSink )\
		pLogger->Info( parameter );\
		END_LOGGER\
	}

#define NET_LOG2( format, parameter )\
	{\
		START_LOGGER( "sink.file", CNetLogFileSink )\
		pLogger->InfoFormat( format, parameter );\
		END_LOGGER\
	}\
	{\
		START_LOGGER( "sink.console", CNetLogConsoleSink )\
		pLogger->InfoFormat( format, parameter );\
		END_LOGGER\
	}

#define NET_LOG3( format, parameter1, parameter2 )\
	{\
		START_LOGGER( "sink.file", CNetLogFileSink )\
		pLogger->InfoFormat( format, parameter1, parameter2 );\
		END_LOGGER\
	}\
	{\
		START_LOGGER( "sink.console", CNetLogConsoleSink )\
		pLogger->InfoFormat( format, parameter1, parameter2 );\
		END_LOGGER\
	}

#define NET_LOG4( format, parameter1, parameter2, parameter3 )\
	{\
		START_LOGGER( "sink.file", CNetLogFileSink )\
		pLogger->InfoFormat( format, parameter1, parameter2, parameter3 );\
		END_LOGGER\
	}\
	{\
		START_LOGGER( "sink.console", CNetLogConsoleSink )\
		pLogger->InfoFormat( format, parameter1, parameter2, parameter3 );\
		END_LOGGER\
	}

#endif	// NETLOG_H

