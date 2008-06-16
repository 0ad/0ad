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

#include "archive.h"

LIB_API PIArchiveReader CreateArchiveReader_Zip(const Path& archivePathname);
LIB_API PIArchiveWriter CreateArchiveWriter_Zip(const Path& archivePathname);

#endif	// #ifndef INCLUDED_ARCHIVE_ZIP
