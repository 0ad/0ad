/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * automatically bundles files into archives in order of access to
 * optimize I/O.
 */

#ifndef INCLUDED_VFS_OPTIMIZER
#define INCLUDED_VFS_OPTIMIZER


extern LibError vfs_opt_rebuild_main_archive(const char* trace_filename, const char* archive_fn_fmt);

extern void vfs_opt_auto_build_cancel();

extern int vfs_opt_auto_build(const char* trace_filename,
	const char* archive_fn_fmt, const char* mini_archive_fn_fmt, bool force_build = false);

extern void vfs_opt_notify_loose_file(const char* atom_fn);
extern void vfs_opt_notify_archived_file(const char* atom_fn);

#endif	// #ifndef INCLUDED_VFS_OPTIMIZER
