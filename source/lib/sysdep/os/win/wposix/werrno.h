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

#ifndef INCLUDED_WERRNO
#define INCLUDED_WERRNO

//
// <errno.h>
//

#include <errno.h>

// this is an exhaustive list of SUSv3 error codes;
// see http://www.opengroup.org/onlinepubs/009695399/basedefs/errno.h.html .

// defined by VC errno.h and SUSv3
// (commented out so that we use VC errno.h's already correct values)
#if 0
#define EPERM            1      // Operation not permitted
#define ENOENT           2      // No such file or directory
#define ESRCH            3      // No such process
#define EINTR            4      // Interrupted system call
#define EIO              5      // I/O error
#define ENXIO            6      // No such device or address
#define E2BIG            7      // Argument list too long
#define ENOEXEC          8      // Exec format error
#define EBADF            9      // Bad file number
#define ECHILD          10      // No child processes
#define EAGAIN          11      // Try again
#define ENOMEM          12      // Out of memory
#define EACCES          13      // Permission denied
#define EFAULT          14      // Bad address
#define EBUSY           16      // Device or resource busy
#define EEXIST          17      // File exists
#define ENODEV          19      // No such device
#define ENOTDIR         20      // Not a directory
#define EISDIR          21      // Is a directory
#define EINVAL          22      // Invalid argument
#define ENFILE          23      // File table overflow
#define EMFILE          24      // Too many open files
#define ENOTTY          25      // Not a typewriter
#define EFBIG           27      // File too large
#define ENOSPC          28      // No space left on device
#define ESPIPE          29      // Illegal seek
#define EMLINK          31      // Too many links
#define EPIPE           32      // Broken pipe
#define EDOM            33      // Math argument out of domain of func
#define ERANGE          34      // Math result not representable
#endif

// defined by VC errno.h and also SUSv3 (with different values)
// (commented out because we must match the values used by the compiled CRT)
#if 0
#define EDEADLK         35      // Resource deadlock would occur
#define ENAMETOOLONG    36      // File name too long
#define ENOLCK          37      // No record locks available
#define ENOSYS          38      // Function not implemented
#define ENOTEMPTY       39      // Directory not empty
#define EILSEQ          84      // Illegal byte sequence
#endif

// defined by winsock2 and also Linux (with different values)
// (values derived from winsock2 WSA* constants minus WSABASEERR)
// update: disabled on newer Boost versions because filesystem drags in boost/cerrno.hpp
#if (!defined(BOOST_VERSION) || BOOST_VERSION <= 103401) && (!MSC_VERSION || MSC_VERSION < 1600)
#define EWOULDBLOCK     35
#define EINPROGRESS     36
#define EALREADY        37
#define ENOTSOCK        38
#define EDESTADDRREQ    39
#define EMSGSIZE        40
#define EPROTOTYPE      41
#define ENOPROTOOPT     42
#define EPROTONOSUPPORT 43
#define EOPNOTSUPP      45
#define EAFNOSUPPORT    47
#define EADDRINUSE      48
#define EADDRNOTAVAIL   49
#define ENETDOWN        50
#define ENETUNREACH     51
#define ENETRESET       52
#define ECONNABORTED    53
#define ECONNRESET      54
#define ENOBUFS         55
#define EISCONN         56
#define ENOTCONN        57
#define ETIMEDOUT       60
#define ECONNREFUSED    61
#define EHOSTUNREACH    65
#define EDQUOT          69
#define ESTALE          70
#endif

// defined by winsock2 but not Linux
// (commented out because they're not portable)
#if 0
#define ESOCKTNOSUPPORT 44
#define EPFNOSUPPORT    46
#define ESHUTDOWN       58
#define ETOOMANYREFS    59
#define ELOOP           62
#define EHOSTDOWN       64
#define EPROCLIM        67
#define EUSERS          68
#define EREMOTE         71
#endif

// defined by Linux but not winsock2
// (commented out because they're not generated on Windows and thus
// probably shouldn't be used)
#if 0
#define ENOMSG          42      // No message of desired type
#define EIDRM           43      // Identifier removed
#define ENOLINK         67      // Reserved
#define EPROTO          71      // Protocol error
#define EMULTIHOP       72      // Reserved
#define EBADMSG         74      // Not a data message
#define EOVERFLOW       75      // Value too large for defined data type
#define ECANCELED       125     // Operation Canceled
#endif

#endif	// #ifndef INCLUDED_WERRNO
