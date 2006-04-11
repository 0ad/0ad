/**
 * =========================================================================
 * File        : vfs_optimizer.h
 * Project     : 0 A.D.
 * Description : automatically bundles files into archives in order of
 *             : access to optimize I/O.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef VFS_OPTIMIZER_H__
#define VFS_OPTIMIZER_H__


extern LibError vfs_opt_rebuild_main_archive(const char* trace_filename, const char* archive_fn_fmt);

extern void vfs_opt_auto_build_cancel();

extern int vfs_opt_auto_build(const char* trace_filename,
	const char* archive_fn_fmt, const char* mini_archive_fn_fmt, bool force_build = false);

extern void vfs_opt_notify_loose_file(const char* atom_fn);
extern void vfs_opt_notify_non_loose_file(const char* atom_fn);

#endif	// #ifndef VFS_OPTIMIZER_H__
