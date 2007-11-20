#include "precompiled.h"
#include "filesystem.h"

#include "path.h"

ERROR_ASSOCIATE(ERR::FILE_ACCESS, "Insufficient access rights to open file", EACCES);
ERROR_ASSOCIATE(ERR::DIR_NOT_FOUND, "Directory not found", ENOENT);
ERROR_ASSOCIATE(ERR::IO, "Error during IO", EIO);
ERROR_ASSOCIATE(ERR::IO_EOF, "Reading beyond end of file", -1);


// rationale for out-of-line dtor: see [Lakos]

IFilesystem::~IFilesystem()
{
}
