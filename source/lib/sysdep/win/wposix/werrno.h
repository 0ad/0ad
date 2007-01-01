#ifndef INCLUDED_WERRNO
#define INCLUDED_WERRNO

//
// <errno.h>
//

#include <errno.h>

// this is an exhaustive list of SUSv3 error codes;
// see http://www.opengroup.org/onlinepubs/009695399/basedefs/errno.h.html .
// .. the following are already correctly defined by VC errno.h:
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
// .. the following are unfortunately defined differently by VC errno.h;
//    we have to stick with those interpretations because they are set
//    by already-compiled CRT code.
#if 0
#define EDEADLK         35      // Resource deadlock would occur
#define ENAMETOOLONG    36      // File name too long
#define ENOLCK          37      // No record locks available
#define ENOSYS          38      // Function not implemented
#define ENOTEMPTY       39      // Directory not empty
#define EILSEQ          84      // Illegal byte sequence
#endif
// .. the following aren't yet defined; we take on the Linux values for
//    simplicity (why assign different values?)
#define ELOOP           40      // Too many symbolic links encountered
#define ENOMSG          42      // No message of desired type
#define EIDRM           43      // Identifier removed
#define EWOULDBLOCK     EAGAIN  // Operation would block
#define ENOLINK         67      // Reserved
#define EPROTO          71      // Protocol error
#define EMULTIHOP       72      // Reserved
#define EBADMSG         74      // Not a data message
#define EOVERFLOW       75      // Value too large for defined data type
#define ENOTSOCK        88      // Socket operation on non-socket
#define EDESTADDRREQ    89      // Destination address required
#define EMSGSIZE        90      // Message too long
#define EPROTOTYPE      91      // Protocol wrong type for socket
#define ENOPROTOOPT     92      // Protocol not available
#define EPROTONOSUPPORT 93      // Protocol not supported
#define EOPNOTSUPP      95      // Operation not supported on transport endpoint
#define EAFNOSUPPORT    97      // Address family not supported by protocol
#define EADDRINUSE      98      // Address already in use
#define EADDRNOTAVAIL   99      // Cannot assign requested address
#define ENETDOWN        100     // Network is down
#define ENETUNREACH     101     // Network is unreachable
#define ENETRESET       102     // Network dropped connection because of reset
#define ECONNABORTED    103     // Software caused connection abort
#define ECONNRESET      104     // Connection reset by peer
#define ENOBUFS         105     // No buffer space available
#define EISCONN         106     // Transport endpoint is already connected
#define ENOTCONN        107     // Transport endpoint is not connected
#define ETIMEDOUT       110     // Connection timed out
#define ECONNREFUSED    111     // Connection refused
#define EHOSTUNREACH    113     // No route to host
#define EALREADY        114     // Operation already in progress
#define EINPROGRESS     115     // Operation now in progress
#define ESTALE          116     // Reserved
#define EDQUOT          122     // Reserved
#define ECANCELED       125     // Operation Canceled

#endif	// #ifndef INCLUDED_WERRNO
