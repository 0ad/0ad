#ifndef ZIP_H__
#define ZIP_H__

#include "archive.h"
#include "file.h"

extern LibError zip_populate_archive(Archive* a, File* f);

extern void zip_fixup_lfh(File* f, ArchiveEntry* ent);


struct ZipArchive;
extern LibError zip_archive_create(const char* zip_filename, ZipArchive** pza);
extern LibError zip_archive_add_file(ZipArchive* za, const ArchiveEntry* ze, void* file_contents);
extern LibError zip_archive_finish(ZipArchive* za);


#endif	// #ifndef ZIP_H__
