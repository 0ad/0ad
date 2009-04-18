/**
 * =========================================================================
 * File        : vfs_optimizer.h
 * Project     : 0 A.D.
 * Description : automatically bundles files into archives in order of
 *             : access to optimize I/O.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_OPTIMIZER
#define INCLUDED_VFS_OPTIMIZER


extern LibError vfs_opt_rebuild_main_archive(const char* trace_filename, const char* archive_fn_fmt);

extern void vfs_opt_auto_build_cancel();

extern int vfs_opt_auto_build(const char* trace_filename,
	const char* archive_fn_fmt, const char* mini_archive_fn_fmt, bool force_build = false);

extern void vfs_opt_notify_loose_file(const char* atom_fn);
extern void vfs_opt_notify_archived_file(const char* atom_fn);

#endif	// #ifndef INCLUDED_VFS_OPTIMIZER
