/**
 * =========================================================================
 * File        : zip.h
 * Project     : 0 A.D.
 * Description : archive backend for Zip files.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ZIP_H__
#define ZIP_H__

struct File;
struct Archive;
struct ArchiveEntry;


// analyse an opened Zip file; call back into archive.cpp to
// populate the Archive object with a list of the files it contains.
// returns INFO_OK on success, ERR_CORRUPTED if file is recognizable as
// a Zip file but invalid, otherwise ERR_UNKNOWN_FORMAT or IO error.
//
// fairly slow - must read Central Directory from disk
// (size ~= 60 bytes*num_files); observed time ~= 80ms.
extern LibError zip_populate_archive(File* f, Archive* a);


// ensures <ent.ofs> points to the actual file contents; it is initially
// the offset of the LFH. we cannot use CDFH filename and extra field
// lengths to skip past LFH since that may not mirror CDFH (has happened).
//
// this is called at file-open time instead of while mounting to
// reduce seeks: since reading the file will typically follow, the
// block cache entirely absorbs the IO cost.
extern void zip_fixup_lfh(File* f, ArchiveEntry* ent);


//
// archive builder backend
//

struct ZipArchive;	// opaque

// create a new Zip archive and return a pointer for use in subsequent
// zip_archive_add_file calls. previous archive file is overwritten.
extern LibError zip_archive_create(const char* zip_filename, ZipArchive** pza);

// add a file (described by ArchiveEntry) to the archive. file_contents
// is the actual file data; its compression method is given in ae->method and
// can be CM_NONE.
// IO cost: writes out <file_contents> to disk (we don't currently attempt
// any sort of write-buffering).
extern LibError zip_archive_add_file(ZipArchive* za, const ArchiveEntry* ae, void* file_contents);

// write out the archive to disk; only hereafter is it valid.
// frees the ZipArchive instance.
// IO cost: writes out Central Directory to disk (about 70 bytes per file).
extern LibError zip_archive_finish(ZipArchive* za);


// for self-test

extern time_t time_t_from_FAT(u32 fat_timedate);
extern u32 FAT_from_time_t(time_t time);

#endif	// #ifndef ZIP_H__
