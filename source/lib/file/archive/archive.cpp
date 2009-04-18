/**
 * =========================================================================
 * File        : archive.cpp
 * Project     : 0 A.D.
 * Description : interface for reading from and creating archives.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "archive.h"

ERROR_ASSOCIATE(ERR::ARCHIVE_UNKNOWN_FORMAT, "Unknown archive format", -1);
ERROR_ASSOCIATE(ERR::ARCHIVE_UNKNOWN_METHOD, "Unknown compression method", -1);

IArchiveReader::~IArchiveReader()
{
}

IArchiveWriter::~IArchiveWriter()
{
}
