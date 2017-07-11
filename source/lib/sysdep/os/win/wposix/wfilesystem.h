/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef INCLUDED_WFILESYSTEM
#define INCLUDED_WFILESYSTEM

//
// sys/stat.h
//

#include <sys/stat.h>	// for S_IFREG etc.

#if MSC_VERSION
typedef unsigned int mode_t;	// defined by MinGW but not VC
#define stat _stat64	// we need 64-bit st_size and time_t
#endif

// permission masks when creating files (_wsopen_s doesn't distinguish
// between owner/user/group)
#define S_IRUSR _S_IREAD
#define S_IRGRP _S_IREAD
#define S_IROTH _S_IREAD
#define S_IWUSR _S_IWRITE
#define S_IWGRP _S_IWRITE
#define S_IWOTH _S_IWRITE
#define S_IXUSR 0
#define S_IXGRP 0
#define S_IXOTH 0
#define S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR)
#define S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP)
#define S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH)

#define S_ISDIR(m) (m & S_IFDIR)
#define S_ISREG(m) (m & S_IFREG)


//
// <unistd.h>
//

extern int read (int fd, void* buf, size_t nbytes);	// thunk
extern int write(int fd, void* buf, size_t nbytes);	// thunk
extern off_t lseek(int fd, off_t ofs, int whence);  // thunk

#endif	// #ifndef INCLUDED_WFILESYSTEM
