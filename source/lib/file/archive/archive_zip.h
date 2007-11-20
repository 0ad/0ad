/**
 * =========================================================================
 * File        : archive_zip.h
 * Project     : 0 A.D.
 * Description : archive backend for Zip files.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_ARCHIVE_ZIP
#define INCLUDED_ARCHIVE_ZIP

struct IArchiveReader;
boost::shared_ptr<IArchiveReader> CreateArchiveReader_Zip(const char* archivePathname);

struct IArchiveWriter;
extern boost::shared_ptr<IArchiveWriter> CreateArchiveWriter_Zip(const char* archivePathname);

#endif	// #ifndef INCLUDED_ARCHIVE_ZIP
