#ifndef INCLUDED_WFILESYSTEM
#define INCLUDED_WFILESYSTEM

//
// sys/stat.h
//

// already defined by MinGW
#if MSC_VERSION
typedef unsigned int mode_t;
#endif

// VC libc includes stat, but it's quite slow.
// we implement our own, but use the CRT struct definition.
// rename the VC function definition to avoid conflict.
/*
#define stat vc_stat
//
// Extra hack for VC++ 2005, since it defines inline stat/fstat
// functions inside stat.h (which get confused by the
// macro-renaming of "stat")
# if MSC_VERSION >= 1400
#  define RC_INVOKED // stat.h only includes stat.inl if "!defined(RC_INVOKED) && !defined(__midl)"
#  include <sys/stat.h>
#  undef RC_INVOKED
# else
#  include <sys/stat.h>
# endif
#undef stat
*/
#include <sys/stat.h>

extern int mkdir(const char*, mode_t);

// currently only sets st_mode (file or dir) and st_size.
//extern int stat(const char*, struct stat*);

#define S_IRWXO 0xFFFF
#define S_IRWXU 0xFFFF
#define S_IRWXG 0xFFFF
// stat.h _S_* values are wrong! disassembly shows _S_IWRITE is 0x80,
// instead of 0x100. define christmas-tree value to be safe.

#define S_ISDIR(m) (m & S_IFDIR)
#define S_ISREG(m) (m & S_IFREG)


//
// dirent.h
//

typedef void DIR;

struct dirent
{
	// note: SUSv3 describes this as a "char array" but of unspecified size.
	// since that precludes using sizeof(), we may as well declare as a
	// pointer to avoid copying in the implementation.
	char* d_name;
};

extern DIR* opendir(const char* name);
extern struct dirent* readdir(DIR*);
extern int closedir(DIR*);

// return status for the file returned by the last successful
// readdir call from the given directory stream.
// currently sets st_size, st_mode, and st_mtime; the rest are zeroed.
// non-portable, but considerably faster than stat(). used by file_enum.
extern int readdir_stat_np(DIR*, struct stat*);


//
// <stdlib.h>
//

extern char* realpath(const char*, char*);


//
// <unistd.h>
//

// values from MS _access() implementation. do not change.
#define F_OK 0
#define R_OK 4
#define W_OK 2
// .. MS implementation doesn't support this distinction.
//    hence, the file is reported executable if it exists.
#define X_OK 0

extern "C" _CRTIMP int access(const char*, int);

extern "C" _CRTIMP int rmdir(const char*);

#endif	// #ifndef INCLUDED_WFILESYSTEM
