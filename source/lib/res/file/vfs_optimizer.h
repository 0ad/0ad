#ifndef VFS_OPTIMIZER_H__
#define VFS_OPTIMIZER_H__


extern LibError vfs_opt_rebuild_main_archive(const char* trace_filename, const char* archive_fn_fmt);

extern void vfs_opt_auto_build_cancel();

extern int vfs_opt_auto_build(const char* trace_filename,
	const char* archive_fn_fmt, const char* mini_archive_fn_fmt, bool force_build = false);

extern void vfs_opt_notify_loose_file(const char* atom_fn);
extern void vfs_opt_notify_non_loose_file(const char* atom_fn);

#endif	// #ifndef VFS_OPTIMIZER_H__
