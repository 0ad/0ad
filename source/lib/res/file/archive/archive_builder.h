/**
 * =========================================================================
 * File        : archive_builder.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_ARCHIVE_BUILDER
#define INCLUDED_ARCHIVE_BUILDER

// array of pointers to VFS filenames (including path), terminated by a
// NULL entry.
typedef const char** Filenames;

struct ZipArchive;

// rationale: this is fairly lightweight and simple, so we don't bother
// making it opaque.
struct ArchiveBuildState
{
	ZipArchive* za;
	uintptr_t ctx;
	Filenames V_fns;
	size_t num_files;	// number of filenames in V_fns (excluding final 0)
	size_t i;
};

extern LibError archive_build_init(const char* P_archive_filename, Filenames V_fns,
	ArchiveBuildState* ab);

// create an archive (overwriting previous file) and fill it with the given
// files. compression method is chosen intelligently based on extension and
// file entropy / achieved compression ratio.
extern int archive_build_continue(ArchiveBuildState* ab);

extern void archive_build_cancel(ArchiveBuildState* ab);

extern LibError archive_build(const char* P_archive_filename, Filenames V_fns);

#endif	// #ifndef INCLUDED_ARCHIVE_BUILDER
