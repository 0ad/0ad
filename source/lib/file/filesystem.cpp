#include "precompiled.h"
#include "filesystem.h"

#include "path.h"

ERROR_ASSOCIATE(ERR::FILE_ACCESS, "Insufficient access rights to open file", EACCES);
ERROR_ASSOCIATE(ERR::DIR_END, "End of directory reached (no more files)", -1);
ERROR_ASSOCIATE(ERR::IO, "Error during IO", EIO);
ERROR_ASSOCIATE(ERR::IO_EOF, "Reading beyond end of file", -1);


// rationale for out-of-line dtors: see [Lakos]

IDirectoryIterator::~IDirectoryIterator()
{
}

IFilesystem::~IFilesystem()
{
}
