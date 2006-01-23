#ifndef ZIP_H__
#define ZIP_H__

#include "archive.h"
#include "file.h"

extern LibError zip_populate_archive(Archive* a, File* f);

extern void zip_fixup_lfh(File* f, ArchiveEntry* ent);

#endif	// #ifndef ZIP_H__
