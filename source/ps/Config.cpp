// Last modified: 6 November 2003 (Mark Thompson)

// TODO: A few changes from VFS -> CFile usage if required.

#include "Config.h"
#include "vfs.h"

using namespace std;

DEFINE_ERROR( PS_FILE_NOT_FOUND, "A data file required by the engine could \
not be found. Check that it exists within the game directory or archive tree." );
DEFINE_ERROR( PS_FILE_LOAD_FAILURE, "One or more data files required by the \
engine could not be loaded. These files may have been deleted or corrupted." );
DEFINE_ERROR( PS_FILE_NODYNAMIC, "A data file was modified during execution, \
but the engine cannot make one or more of these alterations while the game is \
running." );

//--------------------------------------------------------
// SConfigData: Internal file representation
//--------------------------------------------------------

SConfigData::SConfigData( CStr _Filename, void* _Data, LoaderFunction _DynamicLoader )
{
	Filename = _Filename;
	Data = _Data;
	DynamicLoader = _DynamicLoader;
	Timestamp = TIME_UNREGISTERED;
}

//--------------------------------------------------------
// CConfig: Dynamic Data Manager (singleton)
//--------------------------------------------------------

//--------------------------------------------------------
// CConfig::CConfig()
//--------------------------------------------------------

CConfig::CConfig()
{
	Clear();
	Attach( NULL );
	i = m_FileList.begin();
}

//--------------------------------------------------------
// CConfig::Register()
//
//  Add a file to the registered list.
//--------------------------------------------------------

PS_RESULT CConfig::Register( CStr Filename, void* Data, LoaderFunction DynamicLoader )
{
	assert( DynamicLoader != NULL );

	if( m_LogFile )
	{
		CStr Report = _T( "Adding file: " );
		Report += Filename;
		m_LogFile->WriteText( (const TCHAR*)Report );
	}
	
	// Might as well check we can find the thing.
	char filepath[PATH_MAX];

	if( vfs_realpath( Filename, filepath ) )	// This changes filepath to the disk location
												// of the file, if we know it, to speed up
												// checks later.
	{
		if( m_LogFile )
		{
			CStr Error = _T( "File not found on: " );
			Error += Filename;
			m_LogFile->WriteError( (const TCHAR*)Error );
		}
		return( PS_FILE_NOT_FOUND );
	}
	
	m_FileList.push_back( SConfigData( CStr( filepath ), Data, DynamicLoader ) );

	i = m_FileList.begin();

	if( m_LogFile )
	{
		CStr Report = _T( "Found file: " );
		Report += CStr( filepath );
		m_LogFile->WriteText( (const TCHAR*)Report );
	}
	return( PS_OK );
}

//--------------------------------------------------------
// CConfig::Update()
//
//  Check timestamps of files and reload as required.
//--------------------------------------------------------

PS_RESULT CConfig::Update()
{
	_int slice = 0;
	_int failed = 0;
	struct stat FileInfo;

	for( slice = 0; ( i != m_FileList.end() ) && ( slice < CONFIG_SLICE ); i++, slice++ )
	{
		// TODO: CFile change on following line.
		
		if( vfs_stat( i->Filename, &FileInfo ) )
		{
			// We can't find the file; if it exists, it's in an archive.
			if( i->Timestamp )
			{
				// And it's already been loaded once, don't do so again.
				continue;
			}
			// == TIME_UNREGISTERED. Load it, and set the modified date
			// to now so that if it does turn up later on with a time
			// after the start of the program, it will get loaded.
			i->Timestamp = time( NULL );
		}
		else
		{
			if( FileInfo.st_nlink ) 
			{
				// This flag is set by vfs_stat to indicate that the file's
				// gone walkabouts since last we knew its location.
				// Find its new path and copy it back to the data we maintain
				// here to speed up future queries.
				char filepath[PATH_MAX];
				vfs_realpath( i->Filename, filepath );
				if( m_LogFile )
				{
					CStr Report = _T( "File " );
					Report += i->Filename;
					Report += CStr( _T( " moved to: " ) );
					Report += CStr( filepath );
					m_LogFile->WriteText( (const TCHAR*)Report );
				}

				i->Filename = CStr( filepath );
			}
			if( i->Timestamp == FileInfo.st_mtime )
			{
				// This file has the same modification time as it did last
				// time we checked.
				continue;
			}
			i->Timestamp = FileInfo.st_mtime;
		}
		// If we reach here, the file needs to be (re)loaded.
		
		// Note also that polling every frame via _stat() for a file which 
		// either does not exist or exists only in an archive could be a 
		// considerable waste of time, but if not done the game won't pick
		// up on modified versions of archived files moved into the main
		// directory trees. Also, alternatives to polling don't tend to be
		// portable.

		slice--; 
		
		// Reloaded files do not count against the slice quota.

		if( m_LogFile )
		{
			CStr Report = _T( "Reloading file: " );
			Report += i->Filename;
			m_LogFile->WriteText( (const TCHAR*)Report );
		}

		PS_RESULT Result;
		if( ( Result = i->DynamicLoader( i->Filename, i->Data ) ) != PS_OK )
		{
			if( m_LogFile )
			{
				CStr Error = _T( "Load failed on: " );
				Error += CStr( i->Filename );
				Error += CStr( "Load function returned: " );
				Error += CStr( Result );
				m_LogFile->WriteError( (const TCHAR*)Error );
			}
			failed++;
			if( Result != PS_FILE_NODYNAMIC )
				return( PS_FILE_LOAD_FAILURE ); // Oops. Serious problem, bail.
		}
	}
	if( i == m_FileList.end() ) i = m_FileList.begin();
	if( failed )
		return( PS_FILE_NODYNAMIC );
	return( PS_OK );
}

//--------------------------------------------------------
// CConfig::ReloadAll()
//
//  Reloads all files.
//--------------------------------------------------------

PS_RESULT CConfig::ReloadAll()
{
	// Mostly identical to Update(), above.
	_int failed = 0;
	_int notfound = 0; 
	struct stat FileInfo;

	for( i = m_FileList.begin(); i != m_FileList.end(); i++ )
	{
		// TODO: CFile change on following line.
		
		if( vfs_stat( i->Filename, &FileInfo ) )
		{
			// We can't find the file. Seeing as this should reload everything, 
			// check that it exists.
			char filepath[PATH_MAX];
			if( vfs_realpath( i->Filename, filepath ) )
			{
				// Oops.
				notfound++;
				if( m_LogFile )
				{
					CStr Error = _T( "File not found on: " );
					Error += i->Filename;
					m_LogFile->WriteError( (const TCHAR*)Error );
				}
				continue;
			}
			i->Filename = CStr( filepath );
			i->Timestamp = time( NULL );
		}
		else
		{
			if( FileInfo.st_nlink ) 
			{
				// This flag is set by vfs_stat to indicate that the file's
				// gone walkabouts since last we knew its location.
				// Find its new path and copy it back to the data we maintain
				// here to speed up future queries.
				char filepath[PATH_MAX];
				vfs_realpath( i->Filename, filepath );
				if( m_LogFile )
				{
					CStr Report = _T( "File " );
					Report += i->Filename;
					Report += CStr( _T( " moved to: " ) );
					Report += CStr( filepath );
					m_LogFile->WriteText( (const TCHAR*)Report );
				}

				i->Filename = CStr( filepath );
			}
			i->Timestamp = FileInfo.st_mtime;
		}

		// And load them all again...

		if( m_LogFile )
		{
			CStr Report = _T( "Reloading file: " );
			Report += i->Filename;
			m_LogFile->WriteText( (const TCHAR*)Report );
		}

		PS_RESULT Result;
		if( ( Result = i->DynamicLoader( i->Filename, i->Data ) ) != PS_OK )
		{
			if( m_LogFile )
			{
				CStr Error = _T( "Load failed on: " );
				Error += CStr( i->Filename );
				Error += CStr( "Load function returned: " );
				Error += CStr( Result );
				m_LogFile->WriteError( (const TCHAR*)Error );
			}
			failed++;
			if( Result != PS_FILE_NODYNAMIC )
				return( PS_FILE_LOAD_FAILURE ); // Oops. Serious problem, bail.
		}
	}
	if( notfound )
		return( PS_FILE_NOT_FOUND );
	if( failed )
		return( PS_FILE_NODYNAMIC );
	return( PS_OK );
}

//--------------------------------------------------------
// CConfig::Clear()
//
//  Erases registered list.
//--------------------------------------------------------

void CConfig::Clear()
{
	m_FileList.clear();
}

//--------------------------------------------------------
// CConfig::Attach()
//
//  Attaches (or detaches, with a NULL argument) a logfile class.
//--------------------------------------------------------

void CConfig::Attach( CLogFile* LogFile )
{
	m_LogFile = LogFile;
}
