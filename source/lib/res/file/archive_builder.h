/**
 * =========================================================================
 * File        : archive_builder.h
 * Project     : 0 A.D.
 * Description : 
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ARCHIVE_BUILDER_H__
#define ARCHIVE_BUILDER_H__

// array of pointers to VFS filenames (including path), terminated by a
// NULL entry.
typedef const char** Filenames;

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

#endif	// #ifndef ARCHIVE_BUILDER_H__
